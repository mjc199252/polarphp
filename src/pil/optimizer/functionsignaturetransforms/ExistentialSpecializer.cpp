//===--- ExistentialSpecializer.cpp - Specialization of functions -----===//
//===---                 with existential arguments               -----===//
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
//
// Specialize functions with existential parameters to generic ones.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pil-existential-specializer"

#include "polarphp/pil/optimizer/internal/funcsignaturetransforms/ExistentialTransform.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/optimizer/analysis/InterfaceConformanceAnalysis.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/Existential.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"

using namespace polar;

static llvm::cl::opt<bool>
   EnableExistentialSpecializer("enable-existential-specializer",
                                llvm::cl::Hidden,
                                llvm::cl::init(true));

STATISTIC(NumFunctionsWithExistentialArgsSpecialized,
          "Number of functions with existential args specialized");

namespace {

/// ExistentialSpecializer class.
class ExistentialSpecializer : public PILFunctionTransform {

   /// Determine if the current function is a target for existential
   /// specialization of args.
   bool canSpecializeExistentialArgsInFunction(
      FullApplySite &Apply,
      llvm::SmallDenseMap<int, ExistentialTransformArgumentDescriptor>
      &ExistentialArgDescriptor);

   /// Can Callee be specialized?
   bool canSpecializeCalleeFunction(FullApplySite &Apply);

   /// Specialize existential args in function F.
   void specializeExistentialArgsInAppliesWithinFunction(PILFunction &F);

   /// Find concrete type using protocolconformance analysis.
   bool findConcreteTypeFromSoleConformingType(
      PILFunctionArgument *Arg, CanType &ConcreteType);

   /// CallerAnalysis information.
   CallerAnalysis *CA;

   // Determine the set of types a protocol conforms to in whole-module
   // compilation mode.
   InterfaceConformanceAnalysis *PCA;

   ClassHierarchyAnalysis *CHA;
public:
   void run() override {
      auto *F = getFunction();

      /// Don't optimize functions that should not be optimized.
      if (!F->shouldOptimize() || !EnableExistentialSpecializer) {
         return;
      }

      // FIXME: This pass should be able to support ownership.
      if (F->hasOwnership())
         return;

      /// Get CallerAnalysis information handy.
      CA = PM->getAnalysis<CallerAnalysis>();

      /// Get InterfaceConformanceAnalysis.
      PCA = PM->getAnalysis<InterfaceConformanceAnalysis>();

      /// Get ClassHierarchyAnalysis.
      CHA = PM->getAnalysis<ClassHierarchyAnalysis>();
      /// Perform specialization.
      specializeExistentialArgsInAppliesWithinFunction(*F);
   }
};
} // namespace

/// Find concrete type from Sole Conforming Type.
bool ExistentialSpecializer::findConcreteTypeFromSoleConformingType(
   PILFunctionArgument *Arg, CanType &ConcreteType) {
   auto ArgType = Arg->getType();
   auto SwiftArgType = ArgType.getAstType();

   /// Do not handle composition types yet.
   if (isa<InterfaceCompositionType>(SwiftArgType))
      return false;
   assert(ArgType.isExistentialType());
   /// Find the protocol decl.
   auto *PD = dyn_cast<InterfaceDecl>(SwiftArgType->getAnyNominal());
   if (!PD)
      return false;

   // Get SoleConformingType in ConcreteType using InterfaceConformanceAnalysis
   // and ClassHierarchyAnalysis.
   if (!PCA->getSoleConformingType(PD, CHA, ConcreteType))
      return false;
   return true;
}

/// Helper function to ensure that the argument is not InOut or InOut_Aliasable
static bool isNonInoutIndirectArgument(PILValue Arg,
                                       PILArgumentConvention ArgConvention) {
   return !Arg->getType().isObject() && ArgConvention.isIndirectConvention() &&
          ArgConvention != PILArgumentConvention::Indirect_Inout &&
          ArgConvention != PILArgumentConvention::Indirect_InoutAliasable;
}

/// Check if any apply argument meets the criteria for existential
/// specialization.
bool ExistentialSpecializer::canSpecializeExistentialArgsInFunction(
   FullApplySite &Apply,
   llvm::SmallDenseMap<int, ExistentialTransformArgumentDescriptor>
   &ExistentialArgDescriptor) {
   auto *F = Apply.getReferencedFunctionOrNull();
   auto CalleeArgs = F->begin()->getPILFunctionArguments();
   bool returnFlag = false;

   /// Analyze the argument for protocol conformance.  Iterator over the callee's
   /// function arguments.  The same PIL argument index is used for both caller
   /// and callee side arguments.
   auto origCalleeConv = Apply.getOrigCalleeConv();
   assert(Apply.getCalleeArgIndexOfFirstAppliedArg() == 0);
   for (unsigned Idx = 0, Num = CalleeArgs.size(); Idx < Num; ++Idx) {
      auto CalleeArg = CalleeArgs[Idx];
      auto ArgType = CalleeArg->getType();
      auto SwiftArgType = ArgType.getAstType();

      /// Checking for AnyObject and Any is added to ensure that we do not blow up
      /// the code size by specializing to every type that conforms to Any or
      /// AnyObject. In future, we may want to lift these two restrictions in a
      /// controlled way.
      if (!ArgType.isExistentialType() || ArgType.isAnyObject() ||
          SwiftArgType->isAny())
         continue;

      auto ExistentialRepr =
         CalleeArg->getType().getPreferredExistentialRepresentation();
      if (ExistentialRepr != ExistentialRepresentation::Opaque &&
          ExistentialRepr != ExistentialRepresentation::Class)
         continue;

      /// Find the concrete type.
      Operand &ArgOper = Apply.getArgumentRef(Idx);
      CanType ConcreteType =
         ConcreteExistentialInfo(ArgOper.get(), ArgOper.getUser()).ConcreteType;
      auto ArgConvention = F->getConventions().getPILArgumentConvention(Idx);
      /// Find the concrete type, either via sole type or via
      /// findInitExistential..
      CanType SoleConcreteType;
      if (!((F->getModule().isWholeModule() &&
             isNonInoutIndirectArgument(CalleeArg, ArgConvention) &&
             findConcreteTypeFromSoleConformingType(CalleeArg, SoleConcreteType)) ||
            ConcreteType)) {
         LLVM_DEBUG(
            llvm::dbgs()
               << "ExistentialSpecializer Pass: Bail! cannot find ConcreteType "
                  "for call argument to:"
               << F->getName() << " in caller:"
               << Apply.getInstruction()->getParent()->getParent()->getName()
               << "\n";);
         continue;
      }

      /// Determine attributes of the existential addr argument.
      ExistentialTransformArgumentDescriptor ETAD;
      auto paramInfo = origCalleeConv.getParamInfoForPILArg(Idx);
      // The ExistentialSpecializerCloner copies the incoming generic argument
      // into an existential. This won't work if the original argument is
      // mutated. Furthermore, PILCombine would not be able to replace a mutated
      // existential with a concrete value, so the specialization thunk could not
      // be optimized away.
      if (paramInfo.isIndirectMutating())
         continue;

      ETAD.AccessType = paramInfo.isConsumed()
                        ? OpenedExistentialAccess::Mutable
                        : OpenedExistentialAccess::Immutable;
      ETAD.isConsumed = paramInfo.isConsumed();

      /// Save the attributes
      ExistentialArgDescriptor[Idx] = ETAD;
      LLVM_DEBUG(llvm::dbgs()
                    << "ExistentialSpecializer Pass:Function: " << F->getName()
                    << " Arg:" << Idx << " has a concrete type.\n");
      returnFlag |= true;
   }
   return returnFlag;
}

/// Determine if this callee function can be specialized or not.
bool ExistentialSpecializer::canSpecializeCalleeFunction(FullApplySite &Apply) {
   /// Determine the caller of the apply.
   auto *Callee = Apply.getReferencedFunctionOrNull();
   if (!Callee)
      return false;

   /// Callee should be optimizable.
   if (!Callee->shouldOptimize())
      return false;

   /// External function definitions.
   if (!Callee->isDefinition())
      return false;

   // If the callee has ownership enabled, bail.
   //
   // FIXME: We should be able to handle callees that have ownership, but the
   // pass has not been updated yet.
   if (Callee->hasOwnership())
      return false;

   /// Ignore functions with indirect results.
   if (Callee->getConventions().hasIndirectPILResults())
      return false;

   /// Ignore error returning functions.
   if (Callee->getLoweredFunctionType()->hasErrorResult())
      return false;

   /// Do not optimize always_inlinable functions.
   if (Callee->getInlineStrategy() == Inline_t::AlwaysInline)
      return false;

   /// Ignore externally linked functions with public_external or higher
   /// linkage.
   if (is_available_externally(Callee->getLinkage())) {
      return false;
   }

   /// Only choose a select few function representations for specialization.
   switch (Callee->getRepresentation()) {
      case PILFunctionTypeRepresentation::ObjCMethod:
      case PILFunctionTypeRepresentation::Block:
         return false;
      default: break;
   }
   return true;
}

/// Specialize existential args passed as arguments to callees. Iterate over all
/// call sites of the caller F and check for legality to apply existential
/// specialization.
void ExistentialSpecializer::specializeExistentialArgsInAppliesWithinFunction(
   PILFunction &F) {
   bool Changed = false;
   for (auto &BB : F) {
      for (auto It = BB.begin(), End = BB.end(); It != End; ++It) {
         auto *I = &*It;

         /// Is it an apply site?
         FullApplySite Apply = FullApplySite::isa(I);
         if (!Apply)
            continue;

         /// Can the callee be specialized?
         if (!canSpecializeCalleeFunction(Apply)) {
            LLVM_DEBUG(llvm::dbgs() << "ExistentialSpecializer Pass: Bail! Due to "
                                       "canSpecializeCalleeFunction.\n";
                          I->dump(););
            continue;
         }

         auto *Callee = Apply.getReferencedFunctionOrNull();

         /// Handle recursion! Do not modify F right now.
         if (Callee == &F) {
            LLVM_DEBUG(llvm::dbgs() << "ExistentialSpecializer Pass: Bail! Due to "
                                       "recursion.\n";
                          I->dump(););
            continue;
         }

         /// Determine the arguments that can be specialized.
         llvm::SmallDenseMap<int, ExistentialTransformArgumentDescriptor>
            ExistentialArgDescriptor;
         if (!canSpecializeExistentialArgsInFunction(Apply,
                                                     ExistentialArgDescriptor)) {
            LLVM_DEBUG(llvm::dbgs()
                          << "ExistentialSpecializer Pass: Bail! Due to "
                             "canSpecializeExistentialArgsInFunction in function: "
                          << Callee->getName() << " -> abort\n");
            continue;
         }

         LLVM_DEBUG(llvm::dbgs()
                       << "ExistentialSpecializer Pass: Function::"
                       << Callee->getName()
                       << " has an existential argument and can be optimized "
                          "via ExistentialSpecializer\n");

         /// Name Mangler for naming the protocol constrained generic method.
         auto P = demangling::SpecializationPass::FunctionSignatureOpts;
         mangle::FunctionSignatureSpecializationMangler Mangler(
            P, Callee->isSerialized(), Callee);

         /// Save the arguments in a descriptor.
         llvm::SpecificBumpPtrAllocator<ProjectionTreeNode> Allocator;
         llvm::SmallVector<ArgumentDescriptor, 4> ArgumentDescList;
         auto Args = Callee->begin()->getPILFunctionArguments();
         for (unsigned i : indices(Args)) {
            ArgumentDescList.emplace_back(Args[i], Allocator);
         }

         /// This is the function to optimize for existential specilizer.
         LLVM_DEBUG(llvm::dbgs()
                       << "*** Running ExistentialSpecializer Pass on function: "
                       << Callee->getName() << " ***\n");

         /// Instantiate the ExistentialSpecializerTransform pass.
         PILOptFunctionBuilder FuncBuilder(*this);
         ExistentialTransform ET(FuncBuilder, Callee, Mangler, ArgumentDescList,
                                 ExistentialArgDescriptor);

         /// Run the existential specializer pass.
         Changed = ET.run();

         if (Changed) {
            /// Update statistics on the number of functions specialized.
            ++NumFunctionsWithExistentialArgsSpecialized;

            /// Make sure the PM knows about the new specialized inner function.
            addFunctionToPassManagerWorklist(ET.getExistentialSpecializedFunction(),
                                             Callee);

            /// Invalidate analysis results of Callee.
            PM->invalidateAnalysis(Callee,
                                   PILAnalysis::InvalidationKind::Everything);
         }
      }
   }
   return;
}

PILTransform *polar::createExistentialSpecializer() {
   return new ExistentialSpecializer();
}