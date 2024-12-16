//===--- DiagnoseUnreachable.cpp - Diagnose unreachable code --------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pil-diagnose-unreachable"

#include "polarphp/ast/DiagnosticsPIL.h"
#include "polarphp/ast/Expr.h"
#include "polarphp/ast/Pattern.h"
#include "polarphp/ast/Stmt.h"
#include "polarphp/pil/lang/Projection.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILBuilder.h"
#include "polarphp/pil/lang/PILUndef.h"
#include "polarphp/pil/optimizer/passmgr/Passes.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/BasicBlockOptUtils.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"

using namespace polar;

STATISTIC(NumBlocksRemoved, "Number of unreachable basic blocks removed");
STATISTIC(NumInstructionsRemoved, "Number of unreachable instructions removed");
STATISTIC(NumTerminatorsFolded, "Number of terminators folded");
STATISTIC(NumBasicBlockArgsPropagated,
          "Number of basic block arguments propagated");

template<typename...T, typename...U>
static void diagnose(AstContext &Context, SourceLoc loc, Diag<T...> diag,
                     U &&...args) {
   Context.Diags.diagnose(loc,
                          diag, std::forward<U>(args)...);
}

enum class UnreachableKind {
   FoldedBranch,
   FoldedSwitchEnum,
   NoreturnCall,
};

/// Information about a folded conditional branch instruction: it's location
/// and whether the condition evaluated to true or false.
struct UnreachableInfo {
   UnreachableKind Kind;
   /// The location of the instruction that caused the unreachability.
   PILLocation Loc;
   /// If this is the FoldedBranch kind, specifies if the condition is
   /// always true.
   bool CondIsAlwaysTrue;
};

/// \class UnreachableUserCodeReportingState Contains extra state we need to
/// communicate from condition branch folding stage to the unreachable blocks
/// removal stage of the path.
///
/// To report unreachable user code, we detect the blocks that contain user
/// code and are not reachable (along any of the preceding paths). Note that we
/// only want to report the first statement on the unreachable path. Keeping
/// the info about which branch folding had produced the unreachable block makes
/// it possible.
class UnreachableUserCodeReportingState {
public:
   /// The set of top-level blocks that became immediately unreachable due
   /// to conditional branch folding, etc.
   ///
   /// This is a SetVector since several blocks may lead to the same error
   /// report and we iterate through these when producing the diagnostic.
   llvm::SetVector<const PILBasicBlock*> PossiblyUnreachableBlocks;

   /// The set of blocks in which we reported unreachable code errors.
   /// These are used to ensure that we don't issue duplicate reports.
   ///
   /// Note, this set is different from the PossiblyUnreachableBlocks as these
   /// are the blocks that do contain user code and they might not be immediate
   /// successors of a folded branch.
   llvm::SmallPtrSet<const PILBasicBlock*, 2> BlocksWithErrors;

   /// A map from the PossiblyUnreachableBlocks to the folded conditional
   /// branches that caused each of them to be unreachable. This extra info is
   /// used to enhance the diagnostics.
   llvm::DenseMap<const PILBasicBlock*, UnreachableInfo> MetaMap;
};

static void deleteEndBorrows(PILValue v) {
   SmallVector<PILInstruction *, 4> endBorrowList;
   for (auto *use : v->getUses()) {
      if (auto *ebi = dyn_cast<EndBorrowInst>(use->getUser())) {
         endBorrowList.push_back(ebi);
      }
   }
   while (!endBorrowList.empty()) {
      endBorrowList.pop_back_val()->eraseFromParent();
   }
}

/// Propagate/remove basic block input values when all predecessors
/// supply the same arguments.
static void propagateBasicBlockArgs(PILBasicBlock &BB) {
   // This functions would simplify the code as following:
   //
   //   bb0:
   //     br bb2(%1 : $Builtin.Int1, %2 : $Builtin.Int1)
   //   bb1:
   //     br bb2(%1 : $Builtin.Int1, %2 : $Builtin.Int1)
   //   bb2(%3 : $Builtin.Int1, %4 : $Builtin.Int1):
   //     use(%3 : $Builtin.Int1)
   //     use(%4 : $Builtin.Int1)
   // =>
   //   bb0:
   //     br bb2
   //   bb1:
   //     br bb2
   //   bb2:
   //     use(%1 : $Builtin.Int1)
   //     use(%2 : $Builtin.Int1)

   // If there are no predecessors or no arguments, there is nothing to do.
   if (BB.pred_empty() || BB.args_empty())
      return;

   // Check if all the predecessors supply the same arguments to the BB.
   SmallVector<PILValue, 4> Args;
   bool checkArgs = false;
   for (PILBasicBlock::pred_iterator PI = BB.pred_begin(), PE = BB.pred_end();
        PI != PE; ++PI) {
      PILBasicBlock *PredB = *PI;

      // We are only simplifying cases where all predecessors are
      // unconditional branch instructions.
      if (!isa<BranchInst>(PredB->getTerminator()))
         return;

      BranchInst *BI = cast<BranchInst>(PredB->getTerminator());
      unsigned Idx = 0;
      assert(!BI->getArgs().empty());
      for (OperandValueArrayRef::iterator AI = BI->getArgs().begin(),
              AE = BI->getArgs().end();
           AI != AE; ++AI, ++Idx) {
         // When processing the first predecessor, record the arguments.
         if (!checkArgs)
            Args.push_back(*AI);
         else
            // On each subsequent predecessor, check the arguments.
         if (Args[Idx] != *AI)
            return;
      }

      // After the first branch is processed, the arguments vector is populated.
      assert(!Args.empty());
      checkArgs = true;
   }

   // If we've reached this point, the optimization is valid, so optimize.
   // We know that the incoming arguments from all predecessors are the same,
   // so just use them directly and remove the basic block parameters.

   // Drop the arguments from the branch instructions by creating a new branch
   // instruction and deleting the old one.
   llvm::SmallVector<PILInstruction*, 32> ToBeDeleted;
   for (PILBasicBlock::pred_iterator PI = BB.pred_begin(), PE = BB.pred_end();
        PI != PE; ++PI) {
      PILBasicBlock *PredB = *PI;
      BranchInst *BI = cast<BranchInst>(PredB->getTerminator());
      PILBuilderWithScope Bldr(PredB, BI);
      Bldr.createBranch(BI->getLoc(), BI->getDestBB());
      ToBeDeleted.push_back(BI);
   }

   // Drop the parameters from basic blocks and replace all uses with the passed
   // in arguments.
   unsigned Idx = 0;
   for (PILBasicBlock::arg_iterator AI = BB.args_begin(), AE = BB.args_end();
        AI != AE; ++AI, ++Idx) {
      // FIXME: These could be further propagatable now, we might want to move
      // this to CCP and trigger another round of copy propagation.
      PILArgument *Arg = *AI;

      // If this argument is guaranteed and Args[Idx] is a PILFunctionArgument,
      // delete the end_borrow.
      if (Arg->getOwnershipKind() == ValueOwnershipKind::Guaranteed &&
          isa<PILFunctionArgument>(Args[Idx])) {
         deleteEndBorrows(Arg);
      }

      // We were able to fold, so all users should use the new folded value.
      Arg->replaceAllUsesWith(Args[Idx]);
      NumBasicBlockArgsPropagated++;
   }

   // Remove args from the block.
   BB.dropAllArguments();

   // The old branch instructions are no longer used, erase them.
   recursivelyDeleteTriviallyDeadInstructions(ToBeDeleted, true);
   NumInstructionsRemoved += ToBeDeleted.size();
}

static bool constantFoldEnumTerminator(PILBasicBlock &BB,
                                       UnreachableUserCodeReportingState *State,
                                       SwitchEnumInstBase *SUI,
                                       const EnumElementDecl *TheEnumElem,
                                       PILValue value = PILValue(),
                                       PILValue defaultValue = PILValue()) {
   PILBasicBlock *TheSuccessorBlock = nullptr;
   int ReachableBlockIdx = -1;
   for (unsigned Idx = 0; Idx < SUI->getNumCases(); ++Idx) {
      const EnumElementDecl *EI;
      PILBasicBlock *BI;
      std::tie(EI, BI) = SUI->getCase(Idx);
      if (EI == TheEnumElem) {
         TheSuccessorBlock = BI;
         ReachableBlockIdx = Idx;
         break;
      }
   }

   PILBasicBlock *DB = nullptr;
   if (!TheSuccessorBlock) {
      if (SUI->hasDefault()) {
         DB = SUI->getDefaultBB();
         if (!isa<UnreachableInst>(DB->getTerminator())) {
            TheSuccessorBlock = DB;
            ReachableBlockIdx = SUI->getNumCases();
         }
      }
   }

   // Not fully covered switches will be diagnosed later. PILGen represents
   // them with a Default basic block with an unreachable instruction.
   // We are going to produce an error on all unreachable instructions not
   // eliminated by DCE.
   if (!TheSuccessorBlock)
      return false;

   // Replace the switch with a branch to the TheSuccessorBlock.
   PILBuilderWithScope B(&BB, SUI);
   PILLocation Loc = SUI->getLoc();
   if (!TheSuccessorBlock->args_empty()) {
      // If the successor block that we are looking at is the default block,
      // we create an argument not for the enum case, but for the original
      // value.
      PILValue branchOperand;
      if (TheSuccessorBlock != DB) {
         assert(value);
         branchOperand = value;
      } else {
         assert(defaultValue);
         branchOperand = defaultValue;
      }
      B.createBranch(Loc, TheSuccessorBlock, branchOperand);
   } else
      B.createBranch(Loc, TheSuccessorBlock);

   // Produce diagnostic info if we are not within an inlined function or
   // template instantiation.
   // FIXME: Do not report if we are within a template instantiation.
   assert(ReachableBlockIdx >= 0);
   if (Loc.is<RegularLocation>() && State) {
      // Find the first unreachable block in the switch so that we could use
      // it for better diagnostics.
      PILBasicBlock *UnreachableBlock = nullptr;
      if (SUI->getNumCases() > 1) {
         // More than one case.
         UnreachableBlock = (ReachableBlockIdx == 0) ? SUI->getCase(1).second
                                                     : SUI->getCase(0).second;
      } else {
         if (SUI->getNumCases() == 1 && SUI->hasDefault()) {
            // One case and a default.
            UnreachableBlock = (ReachableBlockIdx == 0) ? SUI->getDefaultBB()
                                                        : SUI->getCase(0).second;
         }
      }

      // Generate diagnostic info.
      if (UnreachableBlock &&
          !State->PossiblyUnreachableBlocks.count(UnreachableBlock)) {
         State->PossiblyUnreachableBlocks.insert(UnreachableBlock);
         State->MetaMap.insert(std::pair<const PILBasicBlock *, UnreachableInfo>(
            UnreachableBlock,
            UnreachableInfo{UnreachableKind::FoldedSwitchEnum, Loc, true}));
      }
   }

   LLVM_DEBUG(llvm::dbgs() << "Folding terminator: " << *SUI);
   recursivelyDeleteTriviallyDeadInstructions(SUI, true);
   NumTerminatorsFolded++;
   return true;
}

static InjectEnumAddrInst *
getAllocStackSingleInitializingInjectEnumAddr(SwitchEnumAddrInst *SEAI) {
   auto *stackSlot = dyn_cast<AllocStackInst>(SEAI->getOperand());
   if (!stackSlot)
      return nullptr;

   LLVM_DEBUG(llvm::dbgs() << "Visiting Stack: " << *stackSlot);

   InjectEnumAddrInst *singleInitializer = nullptr;
   InitEnumDataAddrInst *singleInitializerAddr = nullptr;
   SmallVector<Operand *, 16> worklist(stackSlot->use_begin(),
                                       stackSlot->use_end());
   LLVM_DEBUG(POLAR_DEFER { llvm::dbgs() << "Exiting!\n"; });
   while (worklist.size()) {
      auto *op = worklist.pop_back_val();

      LLVM_DEBUG(llvm::dbgs() << "Visiting: " << *op->getUser());
      if (auto *svi = Projection::isAddressProjection(op->getUser())) {
         LLVM_DEBUG(llvm::dbgs() << "Address projection. Continuing\n");
         llvm::copy(svi->getUses(), std::back_inserter(worklist));
         continue;
      }

      auto *user = op->getUser();

      // Skip our self.
      if (user == SEAI) {
         LLVM_DEBUG(llvm::dbgs() << "Skipping SEAI.\n");
         continue;
      }

      if (isa<LoadInst>(user) || isa<LoadBorrowInst>(user) ||
          isa<DeallocStackInst>(user) || isa<DestroyAddrInst>(user)) {
         LLVM_DEBUG(llvm::dbgs() << "Skipping loads/lifetime ends\n");
         continue;
      }

      // If we are reading from the memory we are ok.
      if (auto *cai = dyn_cast<CopyAddrInst>(user)) {
         if (cai->getDest() == op->get() || cai->isTakeOfSrc() == IsTake) {
            LLVM_DEBUG(llvm::dbgs() << "Found cai taking from src. Bailing!\n");
            return nullptr;
         }
         LLVM_DEBUG(llvm::dbgs() << "Skipping!\n");
         continue;
      }

      // Stash the initializer addr. We want to make sure it doesn't
      // escape after we process.
      if (auto *iedai = dyn_cast<InitEnumDataAddrInst>(user)) {
         if (singleInitializerAddr) {
            LLVM_DEBUG(llvm::dbgs() << "Multiple InitEnumDataAddrInst?!\n");
            return nullptr;
         }
         singleInitializerAddr = iedai;
         LLVM_DEBUG(llvm::dbgs() << "Continuing\n");
         continue;
      }

      if (auto *ieai = dyn_cast<InjectEnumAddrInst>(user)) {
         // If single initializer is already set,
         if (singleInitializer) {
            LLVM_DEBUG(llvm::dbgs() << "Multiple InitEnumDataAddrInst?!\n");
            return nullptr;
         }
         singleInitializer = ieai;
         LLVM_DEBUG(llvm::dbgs() << "Continuing\n");
         continue;
      }

      LLVM_DEBUG(llvm::dbgs() << "Bailing at end of loop!\n");
      return nullptr;
   }

   LLVM_DEBUG(llvm::dbgs() << "After Loop\n");

   // If we didn't find a single initializer bail. We were initialized
   // multiple times suggesting we are not actually looking at a PILGen
   // temporary.
   if (!singleInitializer) {
      LLVM_DEBUG(llvm::dbgs() << "Did not find single initializer! Bailing!\n");
      return nullptr;
   }

   // If we didn't have an addr, then it means we had a case without a
   // payload.
   if (!singleInitializerAddr) {
      assert(!singleInitializer->getElement()->hasAssociatedValues());
      LLVM_DEBUG(llvm::dbgs()
                    << "Did not find single initializer addr! Bailing!\n");
      return singleInitializer;
   }

   // Otherwise, make sure we are initialized only once and never
   // escape.
   llvm::copy(singleInitializerAddr->getUses(), std::back_inserter(worklist));
   bool foundInitializer = false;
   while (worklist.size()) {
      auto *op = worklist.pop_back_val();
      LLVM_DEBUG(llvm::dbgs() << "Read only check for: " << *op->getUser());

      // Look through projections.
      if (auto *svi = Projection::isAddressProjection(op->getUser())) {
         llvm::copy(svi->getUses(), std::back_inserter(worklist));
         continue;
      }

      // Skip memory initializing operands. We should only ever see one
      // since PILGen always initializes temporary allocations (our
      // target) that way.
      if (isa<StoreInst>(op->getUser())) {
         if (foundInitializer) {
            LLVM_DEBUG(llvm::dbgs() << "Found multiple initializers! Bailing!\n");
            return nullptr;
         }
         foundInitializer = true;
         continue;
      }

      if (auto *cai = dyn_cast<CopyAddrInst>(op->getUser())) {
         if (cai->getDest() != op->get() ||
             cai->isInitializationOfDest() != IsInitialization) {
            return nullptr;
         }
         if (foundInitializer) {
            LLVM_DEBUG(llvm::dbgs() << "Found multiple initializers! Bailing!\n");
            return nullptr;
         }
         foundInitializer = true;
         continue;
      }

      // Anything else consider unacceptable.
      LLVM_DEBUG(llvm::dbgs() << "Found unknown addr initializer\n");
      return nullptr;
   }

   // If we did not find a single address initializer, bail.
   if (!foundInitializer)
      return nullptr;

   return singleInitializer;
}

static bool constantFoldTerminator(PILBasicBlock &BB,
                                   UnreachableUserCodeReportingState *State) {
   TermInst *TI = BB.getTerminator();

   // Process conditional branches with constant conditions.
   if (auto *CBI = dyn_cast<CondBranchInst>(TI)) {
      PILValue V = CBI->getCondition();
      PILLocation Loc = CBI->getLoc();

      if (IntegerLiteralInst *ConstCond =
         dyn_cast_or_null<IntegerLiteralInst>(V)) {
         PILBuilderWithScope B(&BB, CBI);

         // Determine which of the successors is unreachable and create a new
         // terminator that only branches to the reachable successor.
         PILBasicBlock *UnreachableBlock = nullptr;
         bool CondIsTrue = false;
         if (ConstCond->getValue() == APInt(1, /*value*/ 0, false)) {
            B.createBranch(Loc, CBI->getFalseBB(), CBI->getFalseArgs());
            UnreachableBlock = CBI->getTrueBB();
         } else {
            assert(ConstCond->getValue() == APInt(1, /*value*/ 1, false) &&
                   "Our representation of true/false does not match.");
            B.createBranch(Loc, CBI->getTrueBB(), CBI->getTrueArgs());
            UnreachableBlock = CBI->getFalseBB();
            CondIsTrue = true;
         }
         recursivelyDeleteTriviallyDeadInstructions(TI, true);
         NumInstructionsRemoved++;

         // Produce an unreachable code warning for this basic block if it
         // contains user code (only if we are not within an inlined function or a
         // template instantiation).
         // FIXME: Do not report if we are within a template instantiation.
         // FIXME: Checking for LabeledConditionalStmt is a hack; it's meant to
         // catch cases where we have a #available or similar non-expression
         // condition that was trivially true or false. In these cases we expect
         // the unreachable block to be reachable on another platform and shouldn't
         // emit any warnings about it; if this is not the case it's Sema's
         // responsibility to warn about it.
         if (Loc.is<RegularLocation>() && State &&
             !State->PossiblyUnreachableBlocks.count(UnreachableBlock) &&
             !Loc.isAstNode<LabeledConditionalStmt>()) {
            // If this is the first time we see this unreachable block, store it
            // along with the folded branch info.
            State->PossiblyUnreachableBlocks.insert(UnreachableBlock);
            State->MetaMap.insert(
               std::pair<const PILBasicBlock*, UnreachableInfo>(
                  UnreachableBlock,
                  UnreachableInfo{UnreachableKind::FoldedBranch, Loc, CondIsTrue}));
         }

         NumTerminatorsFolded++;
         return true;
      }
   }

   // Constant fold switch enum.
   //   %1 = enum $Bool, #Bool.false!unionelt
   //   switch_enum %1 : $Bool, case #Bool.true!unionelt: bb1,
   //                            case #Bool.false!unionelt: bb2
   // =>
   //   br bb2
   if (auto *SEI = dyn_cast<SwitchEnumInst>(TI)) {
      if (auto *TheEnum = dyn_cast<EnumInst>(SEI->getOperand())) {
         PILValue operand =
            TheEnum->hasOperand() ? TheEnum->getOperand() : PILValue();
         return constantFoldEnumTerminator(BB, State, SEI, TheEnum->getElement(),
                                           operand /*case*/, TheEnum /*default*/);
      }
   }
   if (auto *SEAI = dyn_cast<SwitchEnumAddrInst>(TI)) {
      // We look for an alloc_stack that never escapes and that is initialized
      // only once. This ensures we only need to find one initialization. This is
      // a common pattern when unwrapping optional values in transparent
      // functions.
      //
      // TODO: This needs a better name.
      if (auto *IEAI = getAllocStackSingleInitializingInjectEnumAddr(SEAI)) {
         return constantFoldEnumTerminator(BB, State, SEAI, IEAI->getElement());
      }
   }

   // Constant fold switch int.
   //   %1 = integer_literal $Builtin.Int64, 2
   //   switch_value %1 : $Builtin.Int64, case 1: bb1, case 2: bb2
   // =>
   //   br bb2
   if (auto *SUI = dyn_cast<SwitchValueInst>(TI)) {
      if (IntegerLiteralInst *SwitchVal =
         dyn_cast<IntegerLiteralInst>(SUI->getOperand())) {
         PILBasicBlock *TheSuccessorBlock = nullptr;
         for (unsigned Idx = 0; Idx < SUI->getNumCases(); ++Idx) {
            APInt AI;
            PILValue EI;
            PILBasicBlock *BI;
            std::tie(EI, BI) = SUI->getCase(Idx);
            // TODO: Check that EI is really an IntegerLiteralInst
            AI = dyn_cast<IntegerLiteralInst>(EI)->getValue();
            if (AI == SwitchVal->getValue())
               TheSuccessorBlock = BI;
         }

         if (!TheSuccessorBlock)
            if (SUI->hasDefault())
               TheSuccessorBlock = SUI->getDefaultBB();

         // Add the branch instruction with the block.
         if (TheSuccessorBlock) {
            PILBuilderWithScope B(&BB, TI);
            B.createBranch(TI->getLoc(), TheSuccessorBlock);
            recursivelyDeleteTriviallyDeadInstructions(TI, true);
            NumTerminatorsFolded++;
            return true;
         }

         // TODO: Warn on unreachable user code here as well.
      }
   }

   return false;
}

/// Check if this instruction corresponds to user-written code.
static bool isUserCode(const PILInstruction *I) {
   PILLocation Loc = I->getLoc();
   if (Loc.isAutoGenerated())
      return false;

   // Branch instructions are not user code. These could belong to the control
   // flow statement we are folding (ex: while loop).
   // Also, unreachable instructions are not user code, they are "expected" in
   // unreachable blocks.
   if ((isa<BranchInst>(I) || isa<UnreachableInst>(I)) &&
       Loc.is<RegularLocation>())
      return false;

   // If the instruction corresponds to user-written return or some other
   // statement, we know it corresponds to user code.
   if (Loc.is<RegularLocation>() || Loc.is<ReturnLocation>()) {
      if (auto *E = Loc.getAsAstNode<Expr>())
         return !E->isImplicit();
      if (auto *D = Loc.getAsAstNode<Decl>())
         return !D->isImplicit();
      if (auto *S = Loc.getAsAstNode<Stmt>())
         return !S->isImplicit();
      if (auto *P = Loc.getAsAstNode<Pattern>())
         return !P->isImplicit();
   }
   return false;
}

static void setOutsideBlockUsesToUndef(PILInstruction *I) {
   if (!I->hasUsesOfAnyResult())
      return;

   PILBasicBlock *BB = I->getParent();
   auto *F = BB->getParent();

   // Replace all uses outside of I's basic block by undef.
   llvm::SmallVector<Operand *, 16> Uses;
   for (auto result : I->getResults())
      Uses.append(result->use_begin(), result->use_end());

   for (auto *Use : Uses)
      if (Use->getUser()->getParent() != BB)
         Use->set(PILUndef::get(Use->get()->getType(), *F));
}

static PILInstruction *getAsCallToNoReturn(PILInstruction *I) {
   if (auto *AI = dyn_cast<ApplyInst>(I))
      if (AI->isCalleeNoReturn())
         return AI;

   if (auto *BI = dyn_cast<BuiltinInst>(I)) {
      if (BI->getModule().isNoReturnBuiltinOrIntrinsic(BI->getName()))
         return BI;
   }

   // These appear in accessors for stored properties with uninhabited
   // type. Since a type containing an uninhabited stored property is
   // itself uninhabited, we treat these identically to fatalError(), etc.
   if (auto *SEI = dyn_cast<StructExtractInst>(I)) {
      if (SEI->getType().getAstType()->isUninhabited())
         return SEI;
   }

   if (auto *SEAI = dyn_cast<StructElementAddrInst>(I)) {
      if (SEAI->getType().getAstType()->isUninhabited())
         return SEAI;
   }

   return nullptr;
}

static PILInstruction *getPrecedingCallToNoReturn(PILBasicBlock &BB) {
   // All the predecessors must satisfy this condition; pick the first
   // as a representative.  PILGen doesn't actually re-use blocks for
   // the normal edge, but it's good to be prepared.
   PILInstruction *first = nullptr;
   for (auto i = BB.pred_begin(), e = BB.pred_end(); i != e; ++i) {
      PILBasicBlock *predBB = *i;

      // As a precaution, bail out if we have a self-loop.  It's not
      // clear what transformations (if any) on naive PILGen output
      // would ever produce that, but still, don't do it.  It's probably
      // only possible in code that's already otherwise provable to be
      // unreachable.
      if (predBB == &BB) return nullptr;

      // The predecessor must be the normal edge from a try_apply
      // that invokes a noreturn function.
      if (auto TAI = dyn_cast<TryApplyInst>((*i)->getTerminator())) {
         if (TAI->isCalleeNoReturn() &&
             TAI->isNormalSuccessorRef(i.getSuccessorRef())) {
            if (!first) first = TAI;
            continue;
         }
      }
      return nullptr;
   }
   return first;
}

static bool simplifyBlocksWithCallsToNoReturn(PILBasicBlock &BB,
                                              UnreachableUserCodeReportingState *State) {
   auto I = BB.begin(), E = BB.end();
   bool DiagnosedUnreachableCode = false;
   PILInstruction *NoReturnCall = nullptr;

   // Collection of all instructions that should be deleted.
   llvm::SmallVector<PILInstruction*, 32> ToBeDeleted;

   // If all of the predecessor blocks end in a try_apply to a noreturn
   // function, the entire block is dead.
   NoReturnCall = getPrecedingCallToNoReturn(BB);

   // Does this block contain a call to a noreturn function?
   while (I != E) {
      auto *CurrentInst = &*I;
      // Move the iterator before we remove instructions to avoid iterator
      // invalidation issues.
      ++I;

      // Remove all instructions following the noreturn call.
      if (NoReturnCall) {

         // We will need to delete the instruction later on.
         ToBeDeleted.push_back(CurrentInst);

         // Diagnose the unreachable code within the same block as the call to
         // noreturn.
         if (isUserCode(CurrentInst) && !DiagnosedUnreachableCode) {
            // If we have an instruction that is an end_borrow, ignore it. This
            // happens when passing a guaranteed argument through generic code paths
            // to no return functions.
            if (!isa<EndBorrowInst>(CurrentInst)) {
               if (NoReturnCall->getLoc().is<RegularLocation>()) {
                  if (!NoReturnCall->getLoc().isAstNode<ExplicitCastExpr>()) {
                     diagnose(BB.getModule().getAstContext(),
                              CurrentInst->getLoc().getSourceLoc(),
                              diag::unreachable_code);
                     diagnose(BB.getModule().getAstContext(),
                              NoReturnCall->getLoc().getSourceLoc(),
                              diag::call_to_noreturn_note);
                     DiagnosedUnreachableCode = true;
                  }
               }
            }
         }

         // We are going to bluntly remove these instructions. Change uses in
         // different basic blocks to undef. This is safe because all control flow
         // created by transparent inlining of functions applications after a call
         // to a 'noreturn' function is control dependent on the call to the
         // noreturn function and therefore dead.
         setOutsideBlockUsesToUndef(CurrentInst);

         NumInstructionsRemoved++;
         continue;
      }

      // Check if this instruction is the first call to noreturn in this block.
      if (!NoReturnCall) {
         NoReturnCall = getAsCallToNoReturn(CurrentInst);
      }
   }

   if (!NoReturnCall)
      return false;

   // If the call is to the 'unreachable' builtin, then remove the call,
   // as it is redundant with the actual unreachable terminator.
   if (auto Builtin = dyn_cast<BuiltinInst>(NoReturnCall)) {
      if (Builtin->getName().str() == "unreachable")
         ToBeDeleted.push_back(NoReturnCall);
   }

   // Record the diagnostic info.
   if (!DiagnosedUnreachableCode &&
       NoReturnCall->getLoc().is<RegularLocation>() && State){
      for (auto SI = BB.succ_begin(), SE = BB.succ_end(); SI != SE; ++SI) {
         PILBasicBlock *UnreachableBlock = *SI;
         if (!State->PossiblyUnreachableBlocks.count(UnreachableBlock)) {
            // If this is the first time we see this unreachable block, store it
            // along with the noreturn call info.
            State->PossiblyUnreachableBlocks.insert(UnreachableBlock);
            State->MetaMap.insert(
               std::pair<const PILBasicBlock*, UnreachableInfo>(
                  UnreachableBlock,
                  UnreachableInfo{UnreachableKind::NoreturnCall,
                                  NoReturnCall->getLoc(), true }));
         }
      }
   }

   auto *Scope = NoReturnCall->getDebugScope();
   recursivelyDeleteTriviallyDeadInstructions(ToBeDeleted, true);
   NumInstructionsRemoved += ToBeDeleted.size();

   // Add an unreachable terminator. The terminator has an invalid source
   // location to signal to the DataflowDiagnostic pass that this code does
   // not correspond to user code.
   // Share the scope with the preceding BB. This causes the debug info to be
   // much smaller and easier to read, but otherwise has no effect.
   PILBuilder B(&BB);
   B.setCurrentDebugScope(Scope);
   B.createUnreachable(ArtificialUnreachableLocation());

   return true;
}

/// Issue an "unreachable code" diagnostic if the blocks contains or
/// leads to another block that contains user code.
///
/// Note, we rely on PILLocation information to determine if PILInstructions
/// correspond to user code.
static bool diagnoseUnreachableBlock(
   const PILBasicBlock &B, PILModule &M,
   const SmallPtrSetImpl<PILBasicBlock *> &Reachable,
   UnreachableUserCodeReportingState *State, const PILBasicBlock *TopLevelB,
   llvm::SmallPtrSetImpl<const PILBasicBlock *> &Visited) {
   if (Visited.count(&B))
      return false;
   Visited.insert(&B);

   assert(State);
   for (auto I = B.begin(), E = B.end(); I != E; ++I) {
      PILLocation Loc = I->getLoc();
      // If we've reached an implicit return, we have not found any user code and
      // can stop searching for it.
      if (Loc.is<ImplicitReturnLocation>() || Loc.isAutoGenerated())
         return false;

      // Check if the instruction corresponds to user-written code, also make
      // sure we don't report an error twice for the same instruction.
      if (isUserCode(&*I) && !State->BlocksWithErrors.count(&B)) {

         // Emit the diagnostic.
         auto BrInfoIter = State->MetaMap.find(TopLevelB);
         assert(BrInfoIter != State->MetaMap.end());
         auto BrInfo = BrInfoIter->second;

         switch (BrInfo.Kind) {
            case (UnreachableKind::FoldedBranch):
               // Emit the diagnostic on the unreachable block and emit the
               // note on the branch responsible for the unreachable code.
               diagnose(M.getAstContext(), Loc.getSourceLoc(), diag::unreachable_code);
               diagnose(M.getAstContext(), BrInfo.Loc.getSourceLoc(),
                        diag::unreachable_code_branch, BrInfo.CondIsAlwaysTrue);
               break;

            case (UnreachableKind::FoldedSwitchEnum): {
               // If we are warning about a switch condition being a constant, the main
               // emphasis should be on the condition (to ensure we have a single
               // message per switch).
               const SwitchStmt *SS = BrInfo.Loc.getAsAstNode<SwitchStmt>();
               if (!SS)
                  break;
               assert(SS);
               const Expr *SE = SS->getSubjectExpr();
               diagnose(M.getAstContext(), SE->getLoc(), diag::switch_on_a_constant);
               diagnose(M.getAstContext(), Loc.getSourceLoc(),
                        diag::unreachable_code_note);
               break;
            }

            case (UnreachableKind::NoreturnCall): {
               // Specialcase when we are warning about unreachable code after a call
               // to a noreturn function.
               if (!BrInfo.Loc.isAstNode<ExplicitCastExpr>()) {
                  assert(BrInfo.Loc.isAstNode<ApplyExpr>());
                  diagnose(M.getAstContext(), Loc.getSourceLoc(),
                           diag::unreachable_code);
                  diagnose(M.getAstContext(), BrInfo.Loc.getSourceLoc(),
                           diag::call_to_noreturn_note);
               }
               break;
            }
         }

         // Record that we've reported this unreachable block to avoid duplicates
         // in the future.
         State->BlocksWithErrors.insert(&B);
         return true;
      }
   }

   // This block could be empty if it's terminator has been folded.
   if (B.empty())
      return false;

   // If we have not found user code in this block, inspect it's successors.
   // Check if at least one of the successors contains user code.
   for (auto I = B.succ_begin(), E = B.succ_end(); I != E; ++I) {
      PILBasicBlock *SB = *I;
      bool HasReachablePred = false;
      for (auto PI = SB->pred_begin(), PE = SB->pred_end(); PI != PE; ++PI) {
         if (Reachable.count(*PI))
            HasReachablePred = true;
      }

      // If all of the predecessors of this successor are unreachable, check if
      // it contains user code.
      if (!HasReachablePred && diagnoseUnreachableBlock(*SB, M, Reachable,
                                                        State, TopLevelB, Visited))
         return true;
   }

   return false;
}

static bool removeUnreachableBlocks(PILFunction &F, PILModule &M,
                                    UnreachableUserCodeReportingState *State) {
   if (F.empty())
      return false;

   SmallPtrSet<PILBasicBlock *, 16> Reachable;
   SmallVector<PILBasicBlock*, 128> Worklist;
   Worklist.push_back(&F.front());
   Reachable.insert(&F.front());

   // Collect all reachable blocks by walking the successors.
   do {
      PILBasicBlock *BB = Worklist.pop_back_val();
      for (auto SI = BB->succ_begin(), SE = BB->succ_end(); SI != SE; ++SI) {
         if (Reachable.insert(*SI).second)
            Worklist.push_back(*SI);
      }
   } while (!Worklist.empty());
   assert(Reachable.size() <= F.size());

   // If everything is reachable, we are done.
   if (Reachable.size() == F.size())
      return false;

   // Diagnose user written unreachable code.
   if (State) {
      for (auto BI = State->PossiblyUnreachableBlocks.begin(),
              BE = State->PossiblyUnreachableBlocks.end(); BI != BE; ++BI) {
         const PILBasicBlock *BB = *BI;
         if (!Reachable.count(BB)) {
            llvm::SmallPtrSet<const PILBasicBlock *, 1> visited;
            diagnoseUnreachableBlock(**BI, M, Reachable, State, BB, visited);
         }
      }
   }

   // Remove references from the dead blocks.
   for (auto I = F.begin(), E = F.end(); I != E; ++I) {
      PILBasicBlock *BB = &*I;
      if (Reachable.count(BB))
         continue;

      // Drop references to other blocks.
      recursivelyDeleteTriviallyDeadInstructions(BB->getTerminator(), true);
      NumInstructionsRemoved++;
   }

   // Delete dead instructions and everything that could become dead after
   // their deletion.
   llvm::SmallVector<PILInstruction*, 32> ToBeDeleted;
   for (auto BI = F.begin(), BE = F.end(); BI != BE; ++BI)
      if (!Reachable.count(&*BI))
         for (auto I = BI->begin(), E = BI->end(); I != E; ++I)
            ToBeDeleted.push_back(&*I);
   recursivelyDeleteTriviallyDeadInstructions(ToBeDeleted, true);
   NumInstructionsRemoved += ToBeDeleted.size();

   // Delete the dead blocks.
   for (auto I = F.begin(), E = F.end(); I != E;)
      if (!Reachable.count(&*I)) {
         I = F.getBlocks().erase(I);
         NumBlocksRemoved++;
      } else
         ++I;

   return true;
}

/// Scan the function for any calls to noreturn functions.  If we find one,
/// change the block to have an unreachable instruction right after it, and
/// diagnose any user code after it as being unreachable.  This pass happens
/// before the definite initialization pass so that it doesn't see infeasible
/// control flow edges.
static void performNoReturnFunctionProcessing(PILFunction &Fn,
                                              PILFunctionTransform *T) {
   LLVM_DEBUG(llvm::errs() << "*** No return function processing: "
                           << Fn.getName() << "\n");
   bool Changed = false;
   for (auto &BB : Fn) {
      // Remove instructions from the basic block after a call to a noreturn
      // function.
      Changed |= simplifyBlocksWithCallsToNoReturn(BB, nullptr);
   }
   if (Changed) {
      removeUnreachableBlocks(Fn);
      T->invalidateAnalysis(PILAnalysis::InvalidationKind::FunctionBody);
   }
}

static void diagnoseUnreachable(PILFunction &Fn) {
   LLVM_DEBUG(llvm::errs() << "*** Diagnose Unreachable processing: "
                           << Fn.getName() << "\n");

   UnreachableUserCodeReportingState State;

   for (auto &BB : Fn) {
      // Simplify the blocks with terminators that rely on constant conditions.
      if (constantFoldTerminator(BB, &State))
         continue;

      // Remove instructions from the basic block after a call to a noreturn
      // function.
      if (simplifyBlocksWithCallsToNoReturn(BB, &State))
         continue;
   }

   // Remove unreachable blocks.
   removeUnreachableBlocks(Fn, Fn.getModule(), &State);

   for (auto &BB : Fn) {
      propagateBasicBlockArgs(BB);
   }

   for (auto &BB : Fn) {
      // Simplify the blocks with terminators that rely on constant conditions.
      if (constantFoldTerminator(BB, &State)) {
         continue;
      }
      // Remove instructions from the basic block after a call to a noreturn
      // function.
      if (simplifyBlocksWithCallsToNoReturn(BB, &State))
         continue;
   }

   // Remove unreachable blocks.
   removeUnreachableBlocks(Fn, Fn.getModule(), &State);
}

// External entry point for other passes, which must do their own invalidation.
void polar::performPILDiagnoseUnreachable(PILModule *M) {
   for (auto &Fn : *M)
      diagnoseUnreachable(Fn);
}

namespace {
class NoReturnFolding : public PILFunctionTransform {
   void run() override {
      // Don't rerun diagnostics on deserialized functions.
      if (getFunction()->wasDeserializedCanonical())
         return;

      performNoReturnFunctionProcessing(*getFunction(), this);
   }
};
} // end anonymous namespace

PILTransform *polar::createNoReturnFolding() {
   return new NoReturnFolding();
}


namespace {
// This pass reruns on deserialized PIL because diagnostic constant propagation
// can expose unreachable blocks which are then removed by this pass.
class DiagnoseUnreachable : public PILFunctionTransform {
   void run() override {
      diagnoseUnreachable(*getFunction());
      invalidateAnalysis(PILAnalysis::InvalidationKind::FunctionBody);
   }
};
} // end anonymous namespace

PILTransform *polar::createDiagnoseUnreachable() {
   return new DiagnoseUnreachable();
}
