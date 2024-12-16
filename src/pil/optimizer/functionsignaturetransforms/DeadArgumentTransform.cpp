//===--- DeadArgumentTransform.cpp ----------------------------------------===//
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

#define DEBUG_TYPE "fso-dead-argument-transform"

#include "polarphp/pil/optimizer/internal/funcsignaturetransforms/FunctionSignatureOpts.h"
#include "polarphp/pil/lang/DebugUtils.h"
#include "llvm/Support/CommandLine.h"

using namespace polar;

static llvm::cl::opt<bool> FSODisableDeadArgument(
   "sil-fso-disable-dead-argument",
   llvm::cl::desc("Do not perform dead argument elimination during FSO. "
                  "Intended only for testing purposes"));

bool FunctionSignatureTransform::DeadArgumentAnalyzeParameters() {
   if (FSODisableDeadArgument)
      return false;

   // Did we decide we should optimize any parameter?
   PILFunction *F = TransformDescriptor.OriginalFunction;
   bool SignatureOptimize = false;
   auto Args = F->begin()->getPILFunctionArguments();
   auto OrigShouldModifySelfArgument =
      TransformDescriptor.shouldModifySelfArgument;
   // Analyze the argument information.
   for (unsigned i : indices(Args)) {
      ArgumentDescriptor &A = TransformDescriptor.ArgumentDescList[i];
      if (!A.PInfo.hasValue()) {
         // It is not an argument. It could be an indirect result.
         continue;
      }

      if (!A.canOptimizeLiveArg()) {
         continue;
      }

      // Check whether argument is dead.
      if (!hasNonTrivialNonDebugTransitiveUsers(Args[i])) {
         A.IsEntirelyDead = true;
         SignatureOptimize = true;
         if (Args[i]->isSelf())
            TransformDescriptor.shouldModifySelfArgument = true;
      }
   }

   if (F->getLoweredFunctionType()->isPolymorphic()) {
      // If the set of dead arguments contains only type arguments,
      // don't remove them, because it would produce a slower code
      // for generic functions.
      bool HasNonTypeDeadArguments = false;
      for (auto &AD : TransformDescriptor.ArgumentDescList) {
         if (AD.IsEntirelyDead &&
             !isa<AnyMetatypeType>(AD.Arg->getType().getAstType())) {
            HasNonTypeDeadArguments = true;
            break;
         }
      }

      if (!HasNonTypeDeadArguments) {
         for (auto &AD : TransformDescriptor.ArgumentDescList) {
            if (AD.IsEntirelyDead) {
               AD.IsEntirelyDead = false;
               break;
            }
         }
         TransformDescriptor.shouldModifySelfArgument =
            OrigShouldModifySelfArgument;
         SignatureOptimize = false;
      }
   }

   return SignatureOptimize;
}

void FunctionSignatureTransform::DeadArgumentTransformFunction() {
   PILFunction *F = TransformDescriptor.OriginalFunction;
   PILBasicBlock *BB = &*F->begin();
   for (const ArgumentDescriptor &AD : TransformDescriptor.ArgumentDescList) {
      if (!AD.IsEntirelyDead)
         continue;
      eraseUsesOfValue(BB->getArgument(AD.Index));
   }
}

void FunctionSignatureTransform::DeadArgumentFinalizeOptimizedFunction() {
   auto *BB = &*TransformDescriptor.OptimizedFunction.get()->begin();
   // Remove any dead argument starting from the last argument to the first.
   for (ArgumentDescriptor &AD : reverse(TransformDescriptor.ArgumentDescList)) {
      if (!AD.IsEntirelyDead)
         continue;
      AD.WasErased = true;
      BB->eraseArgument(AD.Arg->getIndex());
   }
}
