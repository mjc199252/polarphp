//===--------- CopyPropagation.cpp - Remove redundant SSA copies. ---------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
///
/// SSA Copy propagation pass to remove unnecessary copy_value and destroy_value
/// instructions.
///
/// Because this algorithm rewrites copies and destroys without attempting
/// to balance the retain count, it is only sound for ownership-SSA.
///
/// [WIP] This complements -enable-sil-opaque-values. It is not yet enabled by
/// default. It can be enabled once the ownership properties, currently
/// specified as helpers at the top of this file, are formalized via a central
/// API.
///
/// The pass has four steps:
///
/// 1. Find SSA defs that have been copied.
///
/// Then, separately, for each copied def, perform steps 2-4:
///
/// 2. Compute "pruned" liveness of the def and its copies, ignoring original
///    destroys.
///
/// 3. Find the SSA def's final destroy points based on its pruned liveness.
///
/// 4. Rewrite the def's original copies and destroys, inserting new copies
/// where needed.
///
/// The state used by this pass is encapsulated in CopyPropagationState, simply
/// refered to as `pass`:
///
/// - Step 1 initializes `pass.currDef`.
///
/// - Step 2 initializes `pass.liveness`.
///
/// - Step 3 initializes `pass.destroys` and inserts destroy_value instructions.
///
/// - Step 4 deletes original copies and destroys and inserts new copies.
///
/// Example #1: Handle consuming and nonconsuming uses.
///
/// bb0(%arg : @owned $T, %addr : @trivial $*T):
///   %copy = copy_value %arg : $T
///   debug_value %copy : $T
///   store %copy to [init] %addr : $*T
///   debug_value %arg : $T
///   debug_value_addr %addr : $*T
///   destroy_value %arg : $T
///
/// Will be transformed to:
///
/// bb0(%arg : @owned $T, %addr : @trivial $*T):
/// (The original copy is deleted.)
///   debug_value %arg : $T
/// (A new copy_value is inserted before the consuming store.)
///   %copy = copy_value %arg : $T
///   store %copy to [init] %addr : $*T
/// (The non-consuming use now uses the original value.)
///   debug_value %arg : $T
/// (A new destroy is inserted after the last use.)
///   destroy_value %arg : $T
///   debug_value_addr %addr : $*T
/// (The original destroy is deleted.)
///
/// Example #2: Handle control flow.
///
/// bb0(%arg : @owned $T):
///   cond_br %_, bb1, bxb2
///
/// bb1:
///   debug_value %arg : $T
///   %copy = copy_value %arg : $T
///   destroy_value %copy : $T
///   br bb2
///
/// bb2:
///   destroy_value %arg : $T
///
/// Will be transformed to:
///
///  bb0(%arg : @owned $T, %addr : @trivial $*T, %z : @trivial $Builtin.Int1):
///    cond_br %z, bb1, bb3
///
///  bb1:
///  (The critical edge is split.)
///    destroy_value %arg : $T
///    br bb2
///
///  bb3:
///  (The original copy is deleted.)
///    debug_value %arg : $T
///    destroy_value %arg : $T
///    br bb2
///
///  bb2:
///  (The original destroy is deleted.)
///
/// FIXME: When we ban critical edges in PIL, this pass does not need to
/// invalidate any CFG analyses.
///
/// FIXME: PILGen currently emits spurious borrow scopes which block
/// elimination of copies within a scope.
///
/// TODO: Possibly extend this algorithm to better handle aggregates. If
/// ownership PIL consistently uses a `destructure` operation to split
/// aggregates, then the current scalar algorithm should work
/// naturally. However, if we instead need to handle borrow scopes, then this
/// becomes much harder. We would need a DI-style tracker to track the uses of a
/// particular copied def. The single boolean "live" state would be replaced
/// with a bitvector that tracks each tuple element.
///
/// TODO: Cleanup the resulting PIL. Delete instructions with no side effects
/// that produce values which are immediately destroyed after copy propagation.
/// ===----------------------------------------------------------------------===

#define DEBUG_TYPE "copy-propagation"

#include "polarphp/pil/lang/InstructionUtils.h"
#include "polarphp/pil/lang/Projection.h"
#include "polarphp/pil/optimizer/passmgr/Passes.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/CFGOptUtils.h"
#include "polarphp/pil/optimizer/utils/IndexTrie.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Statistic.h"

using namespace polar;

using llvm::DenseMap;
using llvm::DenseSet;
using llvm::SmallSetVector;

STATISTIC(NumCopiesEliminated, "number of copy_value instructions removed");
STATISTIC(NumDestroysEliminated,
          "number of destroy_value instructions removed");
STATISTIC(NumCopiesGenerated, "number of copy_value instructions created");
STATISTIC(NumDestroysGenerated, "number of destroy_value instructions created");
STATISTIC(NumUnknownUsers, "number of functions with unknown users");

//===----------------------------------------------------------------------===//
// Ownership Abstraction.
//
// FIXME: These helpers are only defined in this pass for prototyping.  After
// bootstrapping, they should be moved to a central ownership API (shared by
// PILOwnershipVerifier, etc.).
//
// Categories of owned value users. (U1-U2 apply to any value, O3-O5 only apply
// to owned values).
//
// U1. Use the value instantaneously (copy_value, @guaranteed).
//
// U2. Escape the nontrivial contents of the value (ref_to_unowned,
// unchecked_trivial_bitcast).
//
// O3. Propagate the value without consuming it (mark_dependence, begin_borrow).
//
// O4. Consume the value immediately (store, destroy, @owned, destructure).
//
// O5. Consume the value indirectly via a move (tuple, struct).
// ===---------------------------------------------------------------------===//

// TODO: Figure out how to handle these cases if possible.
static bool isUnknownUse(Operand *use) {
   switch (use->getUser()->getKind()) {
      default:
         return false;
         // FIXME: (Category O3) mark_dependence requires recursion to find all
         // uses. It should be replaced by begin/end dependence.
      case PILInstructionKind::MarkDependenceInst: // Dependent
         // FIXME: (Category O3) ref_tail_addr should require a borrow because it
         // doesn't rely on fix_lifetime like other escaping instructions.
      case PILInstructionKind::RefTailAddrInst:
         // FIXME: (Category O3) dynamic_method_br seems to capture self, presumably
         // propagating lifetime. This should probably borrow self, then be treated
         // like mark_dependence.
      case PILInstructionKind::DynamicMethodBranchInst:
         // FIXME: (Category O3) The ownership verifier says project_box can accept an
         // owned value as a normal use, but it projects the address. That's either an
         // ownership bug or a special case.
      case PILInstructionKind::ProjectBoxInst:
      case PILInstructionKind::ProjectExistentialBoxInst:
         // FIXME: (Category O3) The ownership verifier says open_existential_box can
         // accept an owned value as a normal use, but it projects an address.
      case PILInstructionKind::OpenExistentialBoxInst:
         // Unmanaged operations hopefully don't apply to the same value as CopyValue?
      case PILInstructionKind::UnmanagedRetainValueInst:
      case PILInstructionKind::UnmanagedReleaseValueInst:
      case PILInstructionKind::UnmanagedAutoreleaseValueInst:
         return true;
   }
}

/// Return true if the given owned operand is consumed by the given call.
static bool isAppliedArgConsumed(ApplySite apply, Operand *oper) {
   ParameterConvention paramConv;
   if (oper->get() == apply.getCallee()) {
      assert(oper->getOperandNumber() == 0
             && "function can't be passed to itself");
      paramConv = apply.getSubstCalleeType()->getCalleeConvention();
   } else {
      unsigned argIndex = apply.getCalleeArgIndex(*oper);
      paramConv = apply.getSubstCalleeConv()
         .getParamInfoForPILArg(argIndex)
         .getConvention();
   }
   return isConsumedParameter(paramConv);
}

/// Return true if the given builtin consumes its operand.
static bool isBuiltinArgConsumed(BuiltinInst *BI) {
   const BuiltinInfo &Builtin = BI->getBuiltinInfo();
   switch (Builtin.ID) {
      default:
         llvm_unreachable("Unexpected Builtin with owned value operand.");
         // Extend lifetime without consuming.
      case BuiltinValueKind::ErrorInMain:
      case BuiltinValueKind::UnexpectedError:
      case BuiltinValueKind::WillThrow:
         return false;
         // UnsafeGuaranteed moves the value, which will later be destroyed.
      case BuiltinValueKind::UnsafeGuaranteed:
         return true;
   }
}

/// Return true if the given operand is consumed by its user.
///
/// TODO: Review the semantics of operations that extend the lifetime *without*
/// propagating the value. Ideally, that never happens without borrowing first.
static bool isConsuming(Operand *use) {
   auto *user = use->getUser();
   if (isa<ApplySite>(user))
      return isAppliedArgConsumed(ApplySite(user), use);

   if (auto *BI = dyn_cast<BuiltinInst>(user))
      return isBuiltinArgConsumed(BI);

   switch (user->getKind()) {
      default:
         llvm::dbgs() << *user;
         llvm_unreachable("Unexpected use of a loadable owned value.");

         // Consume the value.
      case PILInstructionKind::AutoreleaseValueInst:
      case PILInstructionKind::DeallocBoxInst:
      case PILInstructionKind::DeallocExistentialBoxInst:
      case PILInstructionKind::DeallocRefInst:
      case PILInstructionKind::DeinitExistentialValueInst:
      case PILInstructionKind::DestroyValueInst:
      case PILInstructionKind::EndLifetimeInst:
      case PILInstructionKind::InitExistentialRefInst:
      case PILInstructionKind::InitExistentialValueInst:
      case PILInstructionKind::KeyPathInst:
      case PILInstructionKind::ReleaseValueInst:
      case PILInstructionKind::ReleaseValueAddrInst:
      case PILInstructionKind::StoreInst:
      case PILInstructionKind::StrongReleaseInst:
      case PILInstructionKind::UnownedReleaseInst:
      case PILInstructionKind::UnconditionalCheckedCastValueInst:
         return true;

         // Terminators must consume their owned values.
      case PILInstructionKind::BranchInst:
      case PILInstructionKind::CheckedCastBranchInst:
      case PILInstructionKind::CheckedCastValueBranchInst:
      case PILInstructionKind::CondBranchInst:
      case PILInstructionKind::ReturnInst:
      case PILInstructionKind::ThrowInst:
         return true;

      case PILInstructionKind::DeallocPartialRefInst:
         return cast<DeallocPartialRefInst>(user)->getInstance() == use->get();

         // Move the value.
      case PILInstructionKind::TupleInst:
      case PILInstructionKind::StructInst:
      case PILInstructionKind::ObjectInst:
      case PILInstructionKind::EnumInst:
      case PILInstructionKind::OpenExistentialRefInst:
      case PILInstructionKind::UpcastInst:
      case PILInstructionKind::UncheckedRefCastInst:
      case PILInstructionKind::ConvertFunctionInst:
      case PILInstructionKind::RefToBridgeObjectInst:
      case PILInstructionKind::BridgeObjectToRefInst:
      case PILInstructionKind::UnconditionalCheckedCastInst:
      case PILInstructionKind::MarkUninitializedInst:
      case PILInstructionKind::UncheckedEnumDataInst:
      case PILInstructionKind::DestructureStructInst:
      case PILInstructionKind::DestructureTupleInst:
         return true;

         // BeginBorrow should already be skipped.
         // EndBorrow extends the lifetime like a normal use.
      case PILInstructionKind::EndBorrowInst:
         return false;

         // Extend the lifetime without borrowing, propagating, or destroying it.
      case PILInstructionKind::BridgeObjectToWordInst:
      case PILInstructionKind::ClassMethodInst:
      case PILInstructionKind::CopyBlockInst:
      case PILInstructionKind::CopyValueInst:
      case PILInstructionKind::DebugValueInst:
      case PILInstructionKind::ExistentialMetatypeInst:
      case PILInstructionKind::FixLifetimeInst:
      case PILInstructionKind::SelectEnumInst:
      case PILInstructionKind::SetDeallocatingInst:
      case PILInstructionKind::StoreWeakInst:
      case PILInstructionKind::ValueMetatypeInst:
         return false;

         // Escape the value. The lifetime must already be enforced via something like
         // fix_lifetime.
      case PILInstructionKind::RefToRawPointerInst:
      case PILInstructionKind::RefToUnmanagedInst:
      case PILInstructionKind::RefToUnownedInst:
      case PILInstructionKind::UncheckedBitwiseCastInst:
      case PILInstructionKind::UncheckedTrivialBitCastInst:
         return false;

         // Dynamic dispatch without capturing self.
      case PILInstructionKind::ObjCMethodInst:
      case PILInstructionKind::ObjCSuperMethodInst:
      case PILInstructionKind::SuperMethodInst:
      case PILInstructionKind::WitnessMethodInst:
         return false;
   }
}

//===----------------------------------------------------------------------===//
// CopyPropagationState: shared state for the pass's analysis and transforms.
//===----------------------------------------------------------------------===//

namespace {
/// LiveWithin blocks have at least one use and/or def within the block, but are
/// not LiveOut.
///
/// LiveOut blocks are live on at least one successor path.
///
/// Dead blocks are either outside of the def's pruned liveness region, or they
/// have not yet been discovered by the liveness computation.
enum IsLive_t { LiveWithin, LiveOut, Dead };

/// Liveness information, which is produced by step #2, computeLiveness, and
/// consumed by step #3, findOrInsertDestroys.
class LivenessInfo {
   // Map of all blocks in which current def is live. True if it is also liveout.
   DenseMap<PILBasicBlock *, bool> liveBlocks;
   // Set of all "interesting" users in this def's live range and whether their
   // used value is consumed. (Non-consuming uses within a block that is already
   // known to be live are uninteresting.)
   DenseMap<PILInstruction *, bool> users;
   // Original points in the CFG where the current value was consumed or
   // destroyed.
   typedef SmallSetVector<PILBasicBlock *, 8> BlockSetVec;
   BlockSetVec originalDestroyBlocks;

public:
   bool empty() {
      assert(!liveBlocks.empty() || users.empty());
      return liveBlocks.empty();
   }

   void clear() {
      liveBlocks.clear();
      users.clear();
      originalDestroyBlocks.clear();
   }

   IsLive_t isBlockLive(PILBasicBlock *bb) const {
      auto liveBlockIter = liveBlocks.find(bb);
      if (liveBlockIter == liveBlocks.end())
         return Dead;
      return liveBlockIter->second ? LiveOut : LiveWithin;
   }

   void markBlockLive(PILBasicBlock *bb, IsLive_t isLive) {
      assert(isLive != Dead && "erasing live blocks isn't implemented.");
      liveBlocks[bb] = (isLive == LiveOut);
   }

   // Return a valid result if the given user was identified as an interesting
   // use of the current def. Returns true if the interesting use consumes the
   // current def.
   Optional<bool> isConsumingUser(PILInstruction *user) const {
      auto useIter = users.find(user);
      if (useIter == users.end())
         return None;
      return useIter->second;
   }

   // Record the user of this operand in the set of interesting users.
   //
   // Note that a user may use the current value from multiple operands. If any
   // of the uses are non-consuming, then we must consider the use itself
   // non-consuming; it cannot be a final destroy point because the value of the
   // non-consuming operand must be kept alive until the end of the
   // user. Consider a call that takes the same value using different
   // conventions:
   //
   //   apply %f(%val, %val) : $(@guaranteed, @owned) -> ()
   //
   // This call cannot be allowed to destroy %val.
   void recordUser(Operand *use) {
      bool consume = isConsuming(use);
      auto iterAndSuccess = users.try_emplace(use->getUser(), consume);
      if (!iterAndSuccess.second)
         iterAndSuccess.first->second &= consume;
   }

   void recordOriginalDestroy(Operand *use) {
      originalDestroyBlocks.insert(use->getUser()->getParent());
   }

   iterator_range<BlockSetVec::const_iterator>
   getOriginalDestroyBlocks() const {
      return originalDestroyBlocks;
   }
};

/// Information about final destroy points, which is produced by step #3,
/// findOrInsertDestroys, and consumed by step #4, rewriteCopies.
///
/// This remains valid during copy rewriting. The only instructions referenced
/// are destroys that cannot be deleted.
///
/// For non-owned values, this remains empty.
class DestroyInfo {
   // Map blocks that contain a final destroy to the destroying instruction.
   DenseMap<PILBasicBlock *, PILInstruction *> finalBlockDestroys;

public:
   bool empty() const { return finalBlockDestroys.empty(); }

   void clear() { finalBlockDestroys.clear(); }

   bool hasFinalDestroy(PILBasicBlock *bb) const {
      return finalBlockDestroys.count(bb);
   }

   // Return true if this instruction is marked as a final destroy point of the
   // current def's live range. A destroy can only be claimed once because
   // instructions like `tuple` can consume the same value via multiple operands.
   bool claimDestroy(PILInstruction *inst) {
      auto destroyPos = finalBlockDestroys.find(inst->getParent());
      if (destroyPos != finalBlockDestroys.end() && destroyPos->second == inst) {
         finalBlockDestroys.erase(destroyPos);
         return true;
      }
      return false;
   }

   void recordFinalDestroy(PILInstruction *inst) {
      finalBlockDestroys[inst->getParent()] = inst;
   }

   void invalidateFinalDestroy(PILInstruction *inst) {
      finalBlockDestroys[inst->getParent()] = inst;
   }
};

/// This pass' shared state.
struct CopyPropagationState {
   PILFunction *F;

   // Per-function invalidation state.
   unsigned invalidation;

   // Current copied def for which this state describes the liveness.
   PILValue currDef;

   // computeLiveness result.
   LivenessInfo liveness;

   // findOrInsertDestroys result.
   DestroyInfo destroys;

   CopyPropagationState(PILFunction *F)
      : F(F), invalidation(PILAnalysis::InvalidationKind::Nothing) {}

   bool isValueOwned() const {
      return currDef.getOwnershipKind() == ValueOwnershipKind::Owned;
   }

   void markInvalid(PILAnalysis::InvalidationKind kind) {
      invalidation |= (unsigned)kind;
   }

   void resetDef(PILValue def) {
      // Do not clear invalidation. It accumulates for an entire function before
      // the PassManager is notified.
      liveness.clear();
      destroys.clear();
      currDef = def;
   }
};
} // namespace

//===----------------------------------------------------------------------===//
// Step 2. Discover "pruned" liveness for a copied def, ignoring copies and
// destroys.
//
// Generate pass.liveness.
// - marks blocks as LiveOut or LiveWithin.
// - records users that may become the last user on that path.
// - records blocks in which currDef may be destroyed.
//
// TODO: Make sure all dependencies are accounted for (mark_dependence,
// ref_element_addr, project_box?). We should have an ownership API so that the
// pass doesn't require any special knowledge of value dependencies.
//===----------------------------------------------------------------------===//

/// Mark blocks live during a reverse CFG traversal from one specific block
/// containing a user.
static void computeUseBlockLiveness(PILBasicBlock *userBB,
                                    CopyPropagationState &pass) {

   // If we are visiting this block, then it is not already LiveOut. Mark it
   // LiveWithin to indicate a liveness boundary within the block.
   pass.liveness.markBlockLive(userBB, LiveWithin);

   SmallVector<PILBasicBlock *, 8> predBBWorklist({userBB});
   while (!predBBWorklist.empty()) {
      PILBasicBlock *bb = predBBWorklist.pop_back_val();
      // The popped `bb` is live; mark all its predecessors LiveOut.
      // pass.currDef was already marked LiveWithin, which terminates traversal.
      for (auto *predBB : bb->getPredecessorBlocks()) {
         switch (pass.liveness.isBlockLive(predBB)) {
            case Dead:
               predBBWorklist.push_back(predBB);
               LLVM_FALLTHROUGH;
            case LiveWithin:
               pass.liveness.markBlockLive(predBB, LiveOut);
               break;
            case LiveOut:
               break;
         }
      }
   }
}

/// Update the current def's liveness based on one specific use operand.
///
/// Terminators consume their owned operands, so they are not live out of the
/// block.
static void computeUseLiveness(Operand *use, CopyPropagationState &pass) {
   auto *bb = use->getUser()->getParent();
   auto isLive = pass.liveness.isBlockLive(bb);
   switch (isLive) {
      case LiveOut:
         // Ignore uses within the pruned liveness.
         return;
      case LiveWithin:
         // Record all uses of blocks on the liveness boundary. Whoever uses this
         // liveness result determines the instruction-level liveness boundary to be
         // the last use in the block.
         pass.liveness.recordUser(use);
         break;
      case Dead: {
         // This use block has not yet been marked live. Mark it and its predecessor
         // blocks live.
         computeUseBlockLiveness(bb, pass);
         // Record all uses of blocks on the liveness boundary. This cannot be
         // determined until after the `computeUseBlockLiveness` CFG traversal
         // because a loop may result in this block being LiveOut.
         if (pass.liveness.isBlockLive(bb) == LiveWithin)
            pass.liveness.recordUser(use);
         break;
      }
   }
}

/// Generate pass.liveness.
/// Return true if successful.
///
/// This finds the "pruned" liveness by recursing through copies and ignoring
/// the original destroys.
///
/// Assumption: No users occur before 'def' in def's BB because this follows the
/// SSA def-use chains without "looking through" any terminators.
static bool computeLiveness(CopyPropagationState &pass) {
   assert(pass.liveness.empty());

   PILBasicBlock *defBB = pass.currDef->getParentBlock();

   pass.liveness.markBlockLive(defBB, LiveWithin);

   SmallSetVector<PILValue, 8> defUseWorkList;
   defUseWorkList.insert(pass.currDef);

   while (!defUseWorkList.empty()) {
      PILValue value = defUseWorkList.pop_back_val();
      for (Operand *use : value->getUses()) {
         auto *user = use->getUser();

         // Bailout if we cannot yet determine the ownership of a use.
         if (isUnknownUse(use)) {
            LLVM_DEBUG(llvm::dbgs() << "Unknown owned value user: "; user->dump());
            ++NumUnknownUsers;
            return false;
         }
         // Recurse through copies.
         if (auto *copy = dyn_cast<CopyValueInst>(user)) {
            defUseWorkList.insert(copy);
            continue;
         }
         // An entire borrow scope is considered a single use that occurs at the
         // point of the end_borrow.
         if (auto *BBI = dyn_cast<BeginBorrowInst>(user)) {
            for (Operand *use : BBI->getUses()) {
               if (auto *EBI = dyn_cast<EndBorrowInst>(use->getUser()))
                  computeUseLiveness(use, pass);
            }
            continue;
         }
         if (isConsuming(use)) {
            pass.liveness.recordOriginalDestroy(use);
            // Destroying a values does not force liveness.
            if (isa<DestroyValueInst>(user))
               continue;
         }
         computeUseLiveness(use, pass);
      }
   }
   return true;
}

//===----------------------------------------------------------------------===//
// Step 3. Find the destroy points of the current def based on the pruned
// liveness computed in Step 2.
//===----------------------------------------------------------------------===//

/// The liveness boundary is at a CFG edge `predBB` -> `succBB`, meaning that
/// `pass.currDef` is live out of at least one other `predBB` successor.
///
/// Create and record a final destroy_value at the beginning of `succBB`
/// (assuming no critical edges).
static void insertDestroyOnCFGEdge(PILBasicBlock *predBB, PILBasicBlock *succBB,
                                   CopyPropagationState &pass) {
   // FIXME: ban critical edges and avoid invalidating CFG analyses.
   auto *destroyBB = splitIfCriticalEdge(predBB, succBB);
   if (destroyBB != succBB)
      pass.markInvalid(PILAnalysis::InvalidationKind::Branches);

   PILBuilderWithScope B(destroyBB->begin());
   auto *DI = B.createDestroyValue(succBB->begin()->getLoc(), pass.currDef);

   pass.destroys.recordFinalDestroy(DI);

   ++NumDestroysGenerated;
   LLVM_DEBUG(llvm::dbgs() << "  Destroy on edge "; DI->dump());

   pass.markInvalid(PILAnalysis::InvalidationKind::Instructions);
}

/// This liveness boundary is within a basic block at the given position.
///
/// Create a final destroy, immediately after `pos`.
static void insertDestroyAtInst(PILBasicBlock::iterator pos,
                                CopyPropagationState &pass) {
   PILBuilderWithScope B(pos);
   auto *DI = B.createDestroyValue((*pos).getLoc(), pass.currDef);
   pass.destroys.recordFinalDestroy(DI);
   ++NumDestroysGenerated;
   LLVM_DEBUG(llvm::dbgs() << "  Destroy at last use "; DI->dump());
   pass.markInvalid(PILAnalysis::InvalidationKind::Instructions);
}

// The pruned liveness boundary is within the given basic block. Find the
// block's last use. If the last use consumes the value, record it as a
// destroy. Otherwise, insert a new destroy_value.
static void findOrInsertDestroyInBlock(PILBasicBlock *bb,
                                       CopyPropagationState &pass) {
   auto *defInst = pass.currDef->getDefiningInstruction();
   auto I = bb->getTerminator()->getIterator();
   while (true) {
      auto *inst = &*I;
      Optional<bool> isConsumingResult = pass.liveness.isConsumingUser(inst);
      if (isConsumingResult.hasValue()) {
         if (isConsumingResult.getValue()) {
            // This consuming use becomes a final destroy.
            pass.destroys.recordFinalDestroy(inst);
            break;
         }
         // Insert a destroy after this non-consuming use.
         assert(inst != bb->getTerminator() && "Terminator must consume operand.");
         insertDestroyAtInst(std::next(I), pass);
         break;
      }
      // This is not a potential last user. Keep scanning.
      // If the original destroy is reached, this is a dead live range. Insert a
      // destroy immediately after the def.
      if (I == bb->begin()) {
         assert(cast<PILArgument>(pass.currDef)->getParent() == bb);
         insertDestroyAtInst(I, pass);
         break;
      }
      --I;
      if (&*I == defInst) {
         insertDestroyAtInst(std::next(I), pass);
         break;
      }
   }
}

/// Populate `pass.finalBlockDestroys` with the final destroy points once copies
/// are eliminated. This only applies to owned values.
///
/// Observations:
/// - The pass.currDef must be postdominated by some subset of its
///   consuming uses, including destroys.
/// - The postdominating consumes cannot be within nested loops.
/// - Any blocks in nested loops are now marked LiveOut.
static void findOrInsertDestroys(CopyPropagationState &pass) {
   assert(!pass.liveness.empty());
   if (!pass.isValueOwned())
      return;

   // Visit each original consuming use or destroy as the starting point for a
   // backward CFG traversal.
   for (auto *originalDestroyBB : pass.liveness.getOriginalDestroyBlocks()) {
      SmallSetVector<PILBasicBlock *, 8> blockPredWorklist;

      // Process each visited block, where succBB == nullptr for the starting
      // point of the CFG traversal.
      auto visitBB = [&](PILBasicBlock *bb, PILBasicBlock *succBB) {
         switch (pass.liveness.isBlockLive(bb)) {
            case LiveOut:
               // If succBB is null, then the original destroy must be an inner
               // nested destroy, so just skip it.
               //
               // Otherwise, this CFG edge is a liveness boundary, so insert a new
               // destroy on the edge.
               if (succBB)
                  insertDestroyOnCFGEdge(bb, succBB, pass);
               break;
            case LiveWithin:
               // The liveness boundary is inside this block. Insert a final destroy
               // inside the block if it doesn't already have one.
               if (!pass.destroys.hasFinalDestroy(bb))
                  findOrInsertDestroyInBlock(bb, pass);
               break;
            case Dead: {
               // Continue searching upward to find the pruned liveness boundary.
               blockPredWorklist.insert(bb);
            }
         }
      };
      // Perform a backward CFG traversal up to the pruned liveness boundary,
      // visiting each block.
      visitBB(originalDestroyBB, nullptr);
      while (!blockPredWorklist.empty()) {
         auto *succBB = blockPredWorklist.pop_back_val();
         for (auto *predBB : succBB->getPredecessorBlocks())
            visitBB(predBB, succBB);
      }
   }
}

//===----------------------------------------------------------------------===//
// Step 4. Rewrite copies and destroys for a single copied definition.
//===----------------------------------------------------------------------===//

/// `pass.currDef` is live across the given consuming use. Copy the value.
static void copyLiveUse(Operand *use, CopyPropagationState &pass) {
   PILInstruction *user = use->getUser();
   PILBuilder B(user->getIterator());
   B.setCurrentDebugScope(user->getDebugScope());

   auto *copy = B.createCopyValue(user->getLoc(), use->get());
   use->set(copy);
   ++NumCopiesGenerated;
   LLVM_DEBUG(llvm::dbgs() << "  Copying at last use "; copy->dump());
   pass.markInvalid(PILAnalysis::InvalidationKind::Instructions);
}

/// Revisit the def-use chain of `pass.currDef`. Mark unneeded original copies
/// and destroys for deletion. Insert new copies for interior uses that require
/// ownership of the used operand.
///
/// TODO: Avoid unnecessary rewrites. Identify copies and destroys that already
/// complement a non-consuming use.
static void rewriteCopies(CopyPropagationState &pass) {
   SmallSetVector<PILInstruction *, 8> instsToDelete;
   SmallSetVector<PILValue, 8> defUseWorklist;

   // Visit each operand in the def-use chain.
   auto visitUse = [&](Operand *use) {
      auto *user = use->getUser();
      // Recurse through copies.
      if (auto *copy = dyn_cast<CopyValueInst>(user)) {
         defUseWorklist.insert(copy);
         return;
      }
      if (auto *destroy = dyn_cast<DestroyValueInst>(user)) {
         // If this destroy was marked as a final destroy, ignore it; otherwise,
         // delete it.
         if (!pass.destroys.claimDestroy(destroy)) {
            instsToDelete.insert(destroy);
            LLVM_DEBUG(llvm::dbgs() << "  Removing "; destroy->dump());
            ++NumDestroysEliminated;
         }
         return;
      }
      // Nonconsuming uses do not need copies and cannot be marked as destroys.
      if (!isConsuming(use))
         return;

      // If this use was marked as a final destroy *and* this is the first
      // consumed operand we have visited, then ignore it. Otherwise, treat it as
      // an "interior" consuming use and insert a copy.
      if (!pass.destroys.claimDestroy(user))
         copyLiveUse(use, pass);
   };

   // Perform a def-use traversal, visiting each use operand.
   defUseWorklist.insert(pass.currDef);
   while (!defUseWorklist.empty()) {
      PILValue value = defUseWorklist.pop_back_val();
      // Recurse through copies then remove them.
      if (auto *copy = dyn_cast<CopyValueInst>(value)) {
         for (auto *use : copy->getUses())
            visitUse(use);
         copy->replaceAllUsesWith(copy->getOperand());
         instsToDelete.insert(copy);
         LLVM_DEBUG(llvm::dbgs() << "  Removing "; copy->dump());
         ++NumCopiesEliminated;
         continue;
      }
      for (Operand *use : value->getUses())
         visitUse(use);
   }
   assert(pass.destroys.empty());

   // Remove the leftover copy_value and destroy_value instructions.
   if (!instsToDelete.empty()) {
      recursivelyDeleteTriviallyDeadInstructions(instsToDelete.takeVector(),
         /*force=*/true);
      pass.markInvalid(PILAnalysis::InvalidationKind::Instructions);
   }
}

//===----------------------------------------------------------------------===//
// CopyPropagation: Top-Level Function Transform.
//===----------------------------------------------------------------------===//

/// TODO: we could strip casts as well, then when recursing through users keep
/// track of the nearest non-copy def. For opaque values, we don't expect to see
/// casts.
static PILValue stripCopies(PILValue v) {
   while (true) {
      v = stripSinglePredecessorArgs(v);

      if (auto *srcCopy = dyn_cast<CopyValueInst>(v)) {
         v = srcCopy->getOperand();
         continue;
      }
      return v;
   }
}

namespace {
class CopyPropagation : public PILFunctionTransform {
   /// The entry point to this function transformation.
   void run() override;
};
} // end anonymous namespace

/// Top-level pass driver.
void CopyPropagation::run() {
   LLVM_DEBUG(llvm::dbgs() << "*** CopyPropagation: " << getFunction()->getName()
                           << "\n");

   // This algorithm fundamentally assumes ownership.
   if (!getFunction()->hasOwnership())
      return;

   // Step 1. Find all copied defs.
   CopyPropagationState pass(getFunction());
   SmallSetVector<PILValue, 16> copiedDefs;
   for (auto &BB : *pass.F) {
      for (auto &I : BB) {
         if (auto *copy = dyn_cast<CopyValueInst>(&I))
            copiedDefs.insert(stripCopies(copy));
      }
   }
   for (auto &def : copiedDefs) {
      pass.resetDef(def);
      // Step 2: computeLiveness
      if (computeLiveness(pass)) {
         // Step 3: findOrInsertDestroys
         findOrInsertDestroys(pass);
         // Invalidate book-keeping before deleting instructions.
         pass.liveness.clear();
         // Step 4: rewriteCopies
         rewriteCopies(pass);
      }
   }
   invalidateAnalysis(PILAnalysis::InvalidationKind(pass.invalidation));
}

PILTransform *polar::createCopyPropagation() { return new CopyPropagation(); }
