//===--- ColdBlockInfo.cpp - Fast/slow path analysis for the PIL CFG ------===//
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

#include "polarphp/pil/optimizer/analysis/ColdBlockInfo.h"
#include "polarphp/pil/optimizer/analysis/DominanceAnalysis.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/ast/SemanticAttrs.h"

using namespace polar;

/// Peek through an extract of Bool.value.
static PILValue getCondition(PILValue C) {
   if (auto *SEI = dyn_cast<StructExtractInst>(C)) {
      if (auto *Struct = dyn_cast<StructInst>(SEI->getOperand()))
         return Struct->getFieldValue(SEI->getField());
      return SEI->getOperand();
   }
   return C;
}

/// \return a BranchHint if this call is a builtin branch hint.
ColdBlockInfo::BranchHint ColdBlockInfo::getBranchHint(PILValue Cond,
                                                       int recursionDepth) {
   // Handle the fully inlined Builtin.
   if (auto *BI = dyn_cast<BuiltinInst>(Cond)) {
      if (BI->getIntrinsicInfo().ID == llvm::Intrinsic::expect) {
         // peek through an extract of Bool.value.
         PILValue ExpectedValue = getCondition(BI->getArguments()[1]);
         if (auto *Literal = dyn_cast<IntegerLiteralInst>(ExpectedValue)) {
            return (Literal->getValue() == 0)
                   ? BranchHint::LikelyFalse : BranchHint::LikelyTrue;
         }
      }
      return BranchHint::None;
   }

   if (auto *Arg = dyn_cast<PILArgument>(Cond)) {
      llvm::SmallVector<std::pair<PILBasicBlock *, PILValue>, 4> InValues;
      if (!Arg->getIncomingPhiValues(InValues))
         return BranchHint::None;

      if (recursionDepth > RecursionDepthLimit)
         return BranchHint::None;

      BranchHint Hint = BranchHint::None;

      // Check all predecessor values which come from non-cold blocks.
      for (auto Pair : InValues) {
         if (isCold(Pair.first, recursionDepth + 1))
            continue;

         auto *IL = dyn_cast<IntegerLiteralInst>(Pair.second);
         if (!IL)
            return BranchHint::None;
         // Check if we have a consistent value for all non-cold predecessors.
         if (IL->getValue().getBoolValue()) {
            if (Hint == BranchHint::LikelyFalse)
               return BranchHint::None;
            Hint = BranchHint::LikelyTrue;
         } else {
            if (Hint == BranchHint::LikelyTrue)
               return BranchHint::None;
            Hint = BranchHint::LikelyFalse;
         }
      }
      return Hint;
   }

   // Handle the @semantic functions used for branch hints.
   auto AI = dyn_cast<ApplyInst>(Cond);
   if (!AI)
      return BranchHint::None;

   if (auto *F = AI->getReferencedFunctionOrNull()) {
      if (F->hasSemanticsAttrs()) {
         // fastpath/slowpath attrs are untested because the inliner luckily
         // inlines them before the downstream calls.
         if (F->hasSemanticsAttr(semantics::SLOWPATH))
            return BranchHint::LikelyFalse;
         else if (F->hasSemanticsAttr(semantics::FASTPATH))
            return BranchHint::LikelyTrue;
      }
   }
   return BranchHint::None;
}

/// \return true if the CFG edge FromBB->ToBB is directly gated by a _slowPath
/// branch hint.
bool ColdBlockInfo::isSlowPath(const PILBasicBlock *FromBB,
                               const PILBasicBlock *ToBB,
                               int recursionDepth) {
   auto *CBI = dyn_cast<CondBranchInst>(FromBB->getTerminator());
   if (!CBI)
      return false;

   PILValue C = getCondition(CBI->getCondition());

   BranchHint hint = getBranchHint(C, recursionDepth);
   if (hint == BranchHint::None)
      return false;

   const PILBasicBlock *ColdTarget =
      (hint == BranchHint::LikelyTrue) ? CBI->getFalseBB() : CBI->getTrueBB();

   return ToBB == ColdTarget;
}

/// \return true if the given block is dominated by a _slowPath branch hint.
///
/// Cache all blocks visited to avoid introducing quadratic behavior.
bool ColdBlockInfo::isCold(const PILBasicBlock *BB, int recursionDepth) {
   auto I = ColdBlockMap.find(BB);
   if (I != ColdBlockMap.end())
      return I->second;

   typedef llvm::DomTreeNodeBase<PILBasicBlock> DomTreeNode;
   DominanceInfo *DT = DA->get(const_cast<PILFunction*>(BB->getParent()));
   DomTreeNode *Node = DT->getNode(const_cast<PILBasicBlock*>(BB));
   // Always consider unreachable code cold.
   if (!Node)
      return true;

   std::vector<const PILBasicBlock*> DomChain;
   DomChain.push_back(BB);
   bool IsCold = false;
   Node = Node->getIDom();
   while (Node) {
      if (isSlowPath(Node->getBlock(), DomChain.back(), recursionDepth)) {
         IsCold = true;
         break;
      }
      auto I = ColdBlockMap.find(Node->getBlock());
      if (I != ColdBlockMap.end()) {
         IsCold = I->second;
         break;
      }
      DomChain.push_back(Node->getBlock());
      Node = Node->getIDom();
   }
   for (auto *ChainBB : DomChain)
      ColdBlockMap[ChainBB] = IsCold;
   return IsCold;
}
