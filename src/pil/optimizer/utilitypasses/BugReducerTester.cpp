//===--- BugReducerTester.cpp ---------------------------------------------===//
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
///
/// \file
///
/// This pass is a testing pass for sil-bug-reducer. It asserts when it visits a
/// function that calls a function specified by an llvm::cl::opt.
///
//===----------------------------------------------------------------------===//

#include "polarphp/pil/lang/PILBuilder.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/optimizer/utils/PILOptFunctionBuilder.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/ApplySite.h"
#include "polarphp/pil/lang/PILLocation.h"
#include "polarphp/pil/lang/PILUndef.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "llvm/Support/CommandLine.h"

using namespace polar;

static llvm::cl::opt<std::string> FunctionTarget(
   "bug-reducer-tester-target-func",
   llvm::cl::desc("Function that when called by an apply should cause "
                  "BugReducerTester to blow up or miscompile if the pass "
                  "visits the apply"));

namespace {
enum class FailureKind {
   OptimizerCrasher,
   RuntimeMiscompile,
   RuntimeCrasher,
   None
};
} // end anonymous namespace

static llvm::cl::opt<FailureKind> TargetFailureKind(
   "bug-reducer-tester-failure-kind",
   llvm::cl::desc("The type of failure to perform"),
   llvm::cl::values(
      clEnumValN(FailureKind::OptimizerCrasher, "opt-crasher",
                 "Crash the optimizer when we see the specified apply"),
      clEnumValN(FailureKind::RuntimeMiscompile, "miscompile",
                 "Delete the target function call to cause a runtime "
                 "miscompile that is not a crasher"),
      clEnumValN(FailureKind::RuntimeCrasher, "runtime-crasher",
                 "Delete the target function call to cause a runtime "
                 "miscompile that is not a crasher")),
   llvm::cl::init(FailureKind::None));


LLVM_ATTRIBUTE_NOINLINE
void THIS_TEST_IS_EXPECTED_TO_CRASH_HERE() {
   llvm_unreachable("Found the target!");
}

namespace {

class BugReducerTester : public PILFunctionTransform {

   // We only want to cause 1 miscompile.
   bool CausedError = false;
   StringRef RuntimeCrasherFunctionName = "bug_reducer_runtime_crasher_func";

   PILFunction *getRuntimeCrasherFunction() {
      assert(TargetFailureKind == FailureKind::RuntimeCrasher);
      llvm::SmallVector<PILResultInfo, 1> ResultInfoArray;
      auto EmptyTupleCanType = getFunction()
         ->getModule()
         .Types.getEmptyTupleType()
         .getAstType();
      ResultInfoArray.push_back(
         PILResultInfo(EmptyTupleCanType, ResultConvention::Unowned));
      auto FuncType = PILFunctionType::get(
         nullptr,
         PILFunctionType::ExtInfo(PILFunctionType::Representation::Thin,
                                  false /*isPseudoGeneric*/, false /*noescape*/,
                                  DifferentiabilityKind::NonDifferentiable,
                                  nullptr /*clangFunctionType*/),
         PILCoroutineKind::None, ParameterConvention::Direct_Unowned,
         ArrayRef<PILParameterInfo>(), ArrayRef<PILYieldInfo>(), ResultInfoArray,
         None, SubstitutionMap(), false,
         getFunction()->getModule().getAstContext());

      PILOptFunctionBuilder FunctionBuilder(*this);
      PILFunction *F = FunctionBuilder.getOrCreateSharedFunction(
         RegularLocation::getAutoGeneratedLocation(), RuntimeCrasherFunctionName,
         FuncType, IsBare, IsNotTransparent, IsSerialized, ProfileCounter(),
         IsNotThunk, IsNotDynamic);
      if (F->isDefinition())
         return F;

      // Create a new block.
      PILBasicBlock *BB = F->createBasicBlock();

      // Insert a builtin int trap. Then return F.
      PILBuilder B(BB);
      B.createBuiltinTrap(RegularLocation::getAutoGeneratedLocation());
      B.createUnreachable(ArtificialUnreachableLocation());
      return F;
   }

   void run() override {
      // If we don't have a target function or we already caused a miscompile,
      // just return.
      if (FunctionTarget.empty() || CausedError)
         return;
      assert(TargetFailureKind != FailureKind::None);
      for (auto &BB : *getFunction()) {
         for (auto II = BB.begin(), IE = BB.end(); II != IE;) {
            // Skip try_apply. We do not support them for now.
            if (isa<TryApplyInst>(&*II)) {
               ++II;
               continue;
            }

            auto FAS = FullApplySite::isa(&*II);
            if (!FAS) {
               ++II;
               continue;
            }

            auto *FRI = dyn_cast<FunctionRefInst>(FAS.getCallee());
            if (!FRI || !FRI->getInitiallyReferencedFunction()->getName().equals(
               FunctionTarget)) {
               ++II;
               continue;
            }

            // Ok, we found the Apply that we want! If we are asked to crash, crash
            // here.
            if (TargetFailureKind == FailureKind::OptimizerCrasher)
               THIS_TEST_IS_EXPECTED_TO_CRASH_HERE();

            // Otherwise, if we are asked to perform a runtime time miscompile,
            // delete the apply target.
            if (TargetFailureKind == FailureKind::RuntimeMiscompile) {
               // Ok, we need to insert a runtime miscompile. Move II to
               // the next instruction and then replace its current value
               // with undef.
               auto *Inst = cast<SingleValueInstruction>(&*II);
               Inst->replaceAllUsesWith(PILUndef::get(Inst->getType(), *getFunction()));
               Inst->eraseFromParent();

               // Mark that we found the miscompile and return so we do not try to
               // visit any more instructions in this function.
               CausedError = true;
               return;
            }

            assert(TargetFailureKind == FailureKind::RuntimeCrasher);
            // Finally, if we reach this point we are being asked to replace the
            // given apply with a new apply that calls the crasher func.
            auto Loc = RegularLocation::getAutoGeneratedLocation();
            PILFunction *RuntimeCrasherFunc = getRuntimeCrasherFunction();
            llvm::dbgs() << "Runtime Crasher Func!\n";
            RuntimeCrasherFunc->dump();
            PILBuilder B(II);
            B.createApply(Loc, B.createFunctionRef(Loc, RuntimeCrasherFunc),
                          SubstitutionMap(),
                          ArrayRef<PILValue>());

            auto *Inst = cast<SingleValueInstruction>(&*II);
            ++II;
            Inst->replaceAllUsesWith(PILUndef::get(Inst->getType(), *getFunction()));
            Inst->eraseFromParent();

            CausedError = true;
            return;
         }
      }
   }
};

} // end anonymous namespace

PILTransform *polar::createBugReducerTester() { return new BugReducerTester(); }
