//===--- GlobalARCSequenceDataflow.cpp - ARC Sequence Dataflow Analysis ---===//
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

#define DEBUG_TYPE "arc-sequence-opts"

#include "polarphp/pil/optimizer/internal/arc/GlobalARCSequenceDataflow.h"
#include "polarphp/pil/optimizer/internal/arc/ARCBBState.h"
#include "polarphp/pil/optimizer/internal/arc/RCStateTransitionVisitors.h"
#include "polarphp/pil/optimizer/analysis/ARCAnalysis.h"
#include "polarphp/pil/optimizer/analysis/PostOrderAnalysis.h"
#include "polarphp/pil/optimizer/analysis/RCIdentityAnalysis.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILSuccessor.h"
#include "polarphp/pil/lang/PILModule.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"

using namespace polar;

//===----------------------------------------------------------------------===//
//                                 Utilities
//===----------------------------------------------------------------------===//

namespace {

using ARCBBState = ARCSequenceDataflowEvaluator::ARCBBState;
using ARCBBStateInfo = ARCSequenceDataflowEvaluator::ARCBBStateInfo;
using ARCBBStateInfoHandle = ARCSequenceDataflowEvaluator::ARCBBStateInfoHandle;

} // end anonymous namespace

//===----------------------------------------------------------------------===//
//                             Top Down Dataflow
//===----------------------------------------------------------------------===//

/// Analyze a single BB for refcount inc/dec instructions.
///
/// If anything was found it will be added to DecToIncStateMap.
///
/// NestingDetected will be set to indicate that the block needs to be
/// reanalyzed if code motion occurs.
bool ARCSequenceDataflowEvaluator::processBBTopDown(ARCBBState &BBState) {
  LLVM_DEBUG(llvm::dbgs() << ">>>> Top Down!\n");

  PILBasicBlock &BB = BBState.getBB();

  bool NestingDetected = false;

  TopDownDataflowRCStateVisitor<ARCBBState> DataflowVisitor(
      RCIA, BBState, DecToIncStateMap, SetFactory);

  // If the current BB is the entry BB, initialize a state corresponding to each
  // of its owned parameters. This enables us to know that if we see a retain
  // before any decrements that the retain is known safe.
  //
  // We do not handle guaranteed parameters here since those are handled in the
  // code in GlobalARCPairingAnalysis. This is because even if we don't do
  // anything, we will still pair the retain, releases and then the guaranteed
  // parameter will ensure it is known safe to remove them.
  if (BB.isEntry()) {
    auto Args = BB.getArguments();
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
      DataflowVisitor.visit(Args[i]);
    }
  }

  // For each instruction I in BB...
  for (auto &I : BB) {

    LLVM_DEBUG(llvm::dbgs() << "VISITING:\n    " << I);

    auto Result = DataflowVisitor.visit(&I);

    // If this instruction can have no further effects on another instructions,
    // continue. This happens for instance if we have cleared all of the state
    // we are tracking.
    if (Result.Kind == RCStateTransitionDataflowResultKind::NoEffects)
      continue;

    // Make sure that we propagate out whether or not nesting was detected.
    NestingDetected |= Result.NestingDetected;

    // This PILValue may be null if we were unable to find a specific RCIdentity
    // that the instruction "visits".
    PILValue Op = Result.RCIdentity;

    // For all other [(PILValue, TopDownState)] we are tracking...
    for (auto &OtherState : BBState.getTopDownStates()) {
      // If the other state's value is blotted, skip it.
      if (!OtherState.hasValue())
        continue;

      // If we visited an increment or decrement successfully (and thus Op is
      // set), if this is the state for this operand, skip it. We already
      // processed it.
      if (Op && OtherState->first == Op)
        continue;

      OtherState->second.updateForSameLoopInst(&I, SetFactory, AA);
    }
  }

  return NestingDetected;
}

void ARCSequenceDataflowEvaluator::mergePredecessors(
    ARCBBStateInfoHandle &DataHandle) {
  bool HasAtLeastOnePred = false;
  llvm::SmallVector<PILBasicBlock *, 4> BBThatNeedInsertPts;

  PILBasicBlock *BB = DataHandle.getBB();
  ARCBBState &BBState = DataHandle.getState();

  // For each successor of BB...
  for (PILBasicBlock *PredBB : BB->getPredecessorBlocks()) {

    // Try to look up the data handle for it. If we don't have any such state,
    // then the predecessor must be unreachable from the entrance and thus is
    // uninteresting to us.
    auto PredDataHandle = getTopDownBBState(PredBB);
    if (!PredDataHandle)
      continue;

    LLVM_DEBUG(llvm::dbgs() << "    Merging Pred: " << PredDataHandle->getID()
                            << "\n");

    // If the predecessor is the head of a backedge in our traversal, clear any
    // state we are tracking now and clear the state of the basic block. There
    // is some sort of control flow here that we do not understand.
    if (PredDataHandle->isBackedge(BB)) {
      BBState.clear();
      break;
    }

    ARCBBState &PredBBState = PredDataHandle->getState();

    // If we found the state but the state is for a trap BB, skip it. Trap BBs
    // leak all reference counts and do not reference semantic objects
    // in any manner.
    //
    // TODO: I think this is a copy paste error, since we a trap BB should have
    // an unreachable at its end. See if this can be removed.
    if (PredBBState.isTrapBB())
      continue;

    if (HasAtLeastOnePred) {
      BBState.mergePredTopDown(PredBBState);
      continue;
    }

    BBState.initPredTopDown(PredBBState);
    HasAtLeastOnePred = true;
  }
}

bool ARCSequenceDataflowEvaluator::processTopDown() {
  bool NestingDetected = false;

  LLVM_DEBUG(llvm::dbgs() << "<<<< Processing Top Down! >>>>\n");

  // For each BB in our reverse post order...
  for (auto *BB : POA->get(&F)->getReversePostOrder()) {
    // We should always have a value here.
    auto BBDataHandle = getTopDownBBState(BB).getValue();

    // This will always succeed since we have an entry for each BB in our RPOT.
    //
    // TODO: When data handles are introduced, print that instead. This code
    // should not be touching BBIDs directly.
    LLVM_DEBUG(llvm::dbgs() << "Processing BB#: " << BBDataHandle.getID()
                            << "\n");

    LLVM_DEBUG(llvm::dbgs() << "Merging Predecessors!\n");
    mergePredecessors(BBDataHandle);

    // Then perform the basic block optimization.
    NestingDetected |= processBBTopDown(BBDataHandle.getState());
  }

  return NestingDetected;
}

//===----------------------------------------------------------------------===//
//                             Bottom Up Dataflow
//===----------------------------------------------------------------------===//

// This is temporary code duplication. This will be removed when Loop ARC is
// finished and Block ARC is removed.
static bool isARCSignificantTerminator(TermInst *TI) {
  switch (TI->getTermKind()) {
  case TermKind::UnreachableInst:
  // br is a forwarding use for its arguments. It cannot in of itself extend
  // the lifetime of an object (just like a phi-node) cannot.
  case TermKind::BranchInst:
  // A cond_br is a forwarding use for its non-operand arguments in a similar
  // way to br. Its operand must be an i1 that has a different lifetime from any
  // ref counted object.
  case TermKind::CondBranchInst:
    return false;
  // Be conservative for now. These actually perform some sort of operation
  // against the operand or can use the value in some way.
  case TermKind::ThrowInst:
  case TermKind::ReturnInst:
  case TermKind::UnwindInst:
  case TermKind::YieldInst:
  case TermKind::TryApplyInst:
  case TermKind::SwitchValueInst:
  case TermKind::SwitchEnumInst:
  case TermKind::SwitchEnumAddrInst:
  case TermKind::DynamicMethodBranchInst:
  case TermKind::CheckedCastBranchInst:
  case TermKind::CheckedCastValueBranchInst:
  case TermKind::CheckedCastAddrBranchInst:
    return true;
  }

  llvm_unreachable("Unhandled TermKind in switch.");
}

/// Analyze a single BB for refcount inc/dec instructions.
///
/// If anything was found it will be added to DecToIncStateMap.
///
/// NestingDetected will be set to indicate that the block needs to be
/// reanalyzed if code motion occurs.
///
/// An epilogue release is a release that post dominates all other uses of a
/// pointer in a function that implies that the pointer is alive up to that
/// point. We "freeze" (i.e. do not attempt to remove or move) such releases if
/// FreezeOwnedArgEpilogueReleases is set. This is useful since in certain cases
/// due to dataflow issues, we cannot properly propagate the last use
/// information. Instead we run an extra iteration of the ARC optimizer with
/// this enabled in a side table so the information gets propagated everywhere in
/// the CFG.
bool ARCSequenceDataflowEvaluator::processBBBottomUp(
    ARCBBState &BBState, bool FreezeOwnedArgEpilogueReleases) {
  LLVM_DEBUG(llvm::dbgs() << ">>>> Bottom Up!\n");
  PILBasicBlock &BB = BBState.getBB();

  bool NestingDetected = false;

  BottomUpDataflowRCStateVisitor<ARCBBState> DataflowVisitor(
      RCIA, EAFI, BBState, FreezeOwnedArgEpilogueReleases, IncToDecStateMap,
      SetFactory);

  // For each terminator instruction I in BB visited in reverse...
  for (auto II = std::next(BB.rbegin()), IE = BB.rend(); II != IE;) {
    PILInstruction &I = *II;
    ++II;

    LLVM_DEBUG(llvm::dbgs() << "VISITING:\n    " << I);

    auto Result = DataflowVisitor.visit(&I);

    // If this instruction can have no further effects on another instructions,
    // continue. This happens for instance if we have cleared all of the state
    // we are tracking.
    if (Result.Kind == RCStateTransitionDataflowResultKind::NoEffects)
      continue;

    // Make sure that we propagate out whether or not nesting was detected.
    NestingDetected |= Result.NestingDetected;

    // This PILValue may be null if we were unable to find a specific RCIdentity
    // that the instruction "visits".
    PILValue Op = Result.RCIdentity;

    // For all other (reference counted value, ref count state) we are
    // tracking...
    for (auto &OtherState : BBState.getBottomupStates()) {
      // If the other state's value is blotted, skip it.
      if (!OtherState.hasValue())
        continue;

      // If this is the state associated with the instruction that we are
      // currently visiting, bail.
      if (Op && OtherState->first == Op)
        continue;

      OtherState->second.updateForSameLoopInst(&I, SetFactory, AA);
    }
  }

  // This is ignoring the possibility that we may have a loop with an
  // interesting terminator but for which, we are going to clear all state
  // (since it is a loop boundary). We may in such a case, be too conservative
  // with our other predecessors. Luckily this cannot happen since cond_br is
  // the only terminator that allows for critical edges and all other
  // "interesting terminators" always having multiple successors. This means
  // that this block could not have multiple predecessors since otherwise, the
  // edge would be broken.
  llvm::TinyPtrVector<PILInstruction *> PredTerminators;
  for (PILBasicBlock *PredBB : BB.getPredecessorBlocks()) {
    auto *TermInst = PredBB->getTerminator();
    if (!isARCSignificantTerminator(TermInst))
      continue;
    PredTerminators.push_back(TermInst);
  }

  for (auto &OtherState : BBState.getBottomupStates()) {
    // If the other state's value is blotted, skip it.
    if (!OtherState.hasValue())
      continue;

    OtherState->second.updateForPredTerminators(PredTerminators,
                                                SetFactory, AA);
  }

  return NestingDetected;
}

void
ARCSequenceDataflowEvaluator::
mergeSuccessors(ARCBBStateInfoHandle &DataHandle) {
  PILBasicBlock *BB = DataHandle.getBB();
  ARCBBState &BBState = DataHandle.getState();

  // For each successor of BB...
  ArrayRef<PILSuccessor> Succs = BB->getSuccessors();
  bool HasAtLeastOneSucc = false;
  for (unsigned i = 0, e = Succs.size(); i != e; ++i) {
    // If it does not have a basic block associated with it...
    auto *SuccBB = Succs[i].getBB();

    // Skip it.
    if (!SuccBB)
      continue;

    // If the BB is the head of a backedge in our traversal, we have
    // hit a loop boundary. In that case, add any instructions we are
    // tracking or instructions that we have seen to the banned
    // instruction list. Clear the instructions we are tracking
    // currently, but leave that we saw a release on them. In a post
    // order, we know that all of a BB's successors will always be
    // visited before the BB, implying we will know if conservatively
    // we saw a release on the pointer going down all paths.
    if (DataHandle.isBackedge(SuccBB)) {
      BBState.clear();
      break;
    }

    // Otherwise, lookup the BBState associated with the successor and merge
    // the successor in. We know this will always succeed.
    auto SuccDataHandle = *getBottomUpBBState(SuccBB);

    ARCBBState &SuccBBState = SuccDataHandle.getState();

    if (SuccBBState.isTrapBB())
      continue;

    if (HasAtLeastOneSucc) {
      BBState.mergeSuccBottomUp(SuccBBState);
      continue;
    }

    BBState.initSuccBottomUp(SuccBBState);
    HasAtLeastOneSucc = true;
  }
}

bool ARCSequenceDataflowEvaluator::processBottomUp(
    bool FreezeOwnedArgEpilogueReleases) {
  bool NestingDetected = false;

  LLVM_DEBUG(llvm::dbgs() << "<<<< Processing Bottom Up! >>>>\n");

  // For each BB in our post order...
  for (auto *BB : POA->get(&F)->getPostOrder()) {
    // Grab the BBState associated with it and set it to be the current BB.
    auto BBDataHandle = *getBottomUpBBState(BB);

    // This will always succeed since we have an entry for each BB in our post
    // order.
    LLVM_DEBUG(llvm::dbgs() << "Processing BB#: " << BBDataHandle.getID()
                            << "\n");

    LLVM_DEBUG(llvm::dbgs() << "Merging Successors!\n");
    mergeSuccessors(BBDataHandle);

    // Then perform the basic block optimization.
    NestingDetected |= processBBBottomUp(BBDataHandle.getState(),
                                         FreezeOwnedArgEpilogueReleases);
  }

  return NestingDetected;
}

//===----------------------------------------------------------------------===//
//                 Top Level ARC Sequence Dataflow Evaluator
//===----------------------------------------------------------------------===//

ARCSequenceDataflowEvaluator::ARCSequenceDataflowEvaluator(
    PILFunction &F, AliasAnalysis *AA, PostOrderAnalysis *POA,
    RCIdentityFunctionInfo *RCIA, EpilogueARCFunctionInfo *EAFI,
    ProgramTerminationFunctionInfo *PTFI,
    BlotMapVector<PILInstruction *, TopDownRefCountState> &DecToIncStateMap,
    BlotMapVector<PILInstruction *, BottomUpRefCountState> &IncToDecStateMap)
    : F(F), AA(AA), POA(POA), RCIA(RCIA), EAFI(EAFI),
      DecToIncStateMap(DecToIncStateMap), IncToDecStateMap(IncToDecStateMap),
      Allocator(), SetFactory(Allocator),
      // We use a malloced pointer here so we don't need to expose
      // ARCBBStateInfo in the header.
      BBStateInfo(new ARCBBStateInfo(&F, POA, PTFI)) {}

bool ARCSequenceDataflowEvaluator::run(bool FreezeOwnedReleases) {
  bool NestingDetected = processBottomUp(FreezeOwnedReleases);
  NestingDetected |= processTopDown();
  return NestingDetected;
}

// We put the destructor here so we don't need to expose the type of
// BBStateInfo to the outside world.
ARCSequenceDataflowEvaluator::~ARCSequenceDataflowEvaluator() = default;

void ARCSequenceDataflowEvaluator::clear() { BBStateInfo->clear(); }

llvm::Optional<ARCBBStateInfoHandle>
ARCSequenceDataflowEvaluator::getBottomUpBBState(PILBasicBlock *BB) {
  return BBStateInfo->getBottomUpBBHandle(BB);
}

llvm::Optional<ARCBBStateInfoHandle>
ARCSequenceDataflowEvaluator::getTopDownBBState(PILBasicBlock *BB) {
  return BBStateInfo->getTopDownBBHandle(BB);
}
