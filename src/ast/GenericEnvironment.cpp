//===--- GenericEnvironment.cpp - GenericEnvironment AST ------------------===//
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
// This file implements the GenericEnvironment class.
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/GenericEnvironment.h"
#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/GenericSignatureBuilder.h"
#include "polarphp/ast/InterfaceConformance.h"

namespace polar {

size_t GenericEnvironment::numTrailingObjects(OverloadToken <Type>) const {
   return Signature->getGenericParams().size();
}

/// Retrieve the array containing the context types associated with the
/// generic parameters, stored in parallel with the generic parameters of the
/// generic signature.
MutableArrayRef <Type> GenericEnvironment::getContextTypes() {
   return MutableArrayRef<Type>(getTrailingObjects<Type>(),
                                Signature->getGenericParams().size());
}

/// Retrieve the array containing the context types associated with the
/// generic parameters, stored in parallel with the generic parameters of the
/// generic signature.
ArrayRef <Type> GenericEnvironment::getContextTypes() const {
   return ArrayRef<Type>(getTrailingObjects<Type>(),
                         Signature->getGenericParams().size());
}

TypeArrayView <GenericTypeParamType>
GenericEnvironment::getGenericParams() const {
   return Signature->getGenericParams();
}

GenericEnvironment::GenericEnvironment(GenericSignature signature,
                                       GenericSignatureBuilder *builder)
   : Signature(signature), Builder(builder) {
   // Clear out the memory that holds the context types.
   std::uninitialized_fill(getContextTypes().begin(), getContextTypes().end(),
                           Type());
}

void GenericEnvironment::addMapping(GenericParamKey key,
                                    Type contextType) {
   // Find the index into the parallel arrays of generic parameters and
   // context types.
   auto genericParams = Signature->getGenericParams();
   unsigned index = key.findIndexIn(genericParams);
   assert(genericParams[index] == key && "Bad generic parameter");

   // Add the mapping from the generic parameter to the context type.
   assert(getContextTypes()[index].isNull() && "Already recoded this mapping");
   getContextTypes()[index] = contextType;
}

Optional <Type> GenericEnvironment::getMappingIfPresent(
   GenericParamKey key) const {
   // Find the index into the parallel arrays of generic parameters and
   // context types.
   auto genericParams = Signature->getGenericParams();
   unsigned index = key.findIndexIn(genericParams);
   assert(genericParams[index] == key && "Bad generic parameter");

   if (auto type = getContextTypes()[index])
      return type;

   return None;
}

Type GenericEnvironment::mapTypeIntoContext(GenericEnvironment *env,
                                            Type type) {
   assert(!type->hasArchetype() && "already have a contextual type");

   if (!env)
      return type.substDependentTypesWithErrorTypes();

   return env->mapTypeIntoContext(type);
}

Type MapTypeOutOfContext::operator()(SubstitutableType *type) const {
   auto archetype = cast<ArchetypeType>(type);
   if (isa<OpaqueTypeArchetypeType>(archetype->getRoot()))
      return Type();

   return archetype->getInterfaceType();
}

Type TypeBase::mapTypeOutOfContext() {
   assert(!hasTypeParameter() && "already have an interface type");
   return Type(this).subst(MapTypeOutOfContext(),
                           MakeAbstractConformanceForGenericType(),
                           SubstFlags::AllowLoweredTypes);
}

Type QueryInterfaceTypeSubstitutions::operator()(SubstitutableType *type) const {
   if (auto gp = type->getAs<GenericTypeParamType>()) {
      // Find the index into the parallel arrays of generic parameters and
      // context types.
      auto genericParams = self->Signature->getGenericParams();
      GenericParamKey key(gp);

      // Make sure that this generic parameter is from this environment.
      unsigned index = key.findIndexIn(genericParams);
      if (index == genericParams.size() || genericParams[index] != key)
         return Type();

      // If the context type isn't already known, lazily create it.
      Type contextType = self->getContextTypes()[index];
      if (!contextType) {
         assert(self->Builder && "Missing generic signature builder for lazy query");
         auto equivClass =
            self->Builder->resolveEquivalenceClass(
               type,
               ArchetypeResolutionKind::CompleteWellFormed);

         auto mutableSelf = const_cast<GenericEnvironment *>(self);
         contextType = equivClass->getTypeInContext(*mutableSelf->Builder,
                                                    mutableSelf);

         // FIXME: Redundant mapping from key -> index.
         if (self->getContextTypes()[index].isNull())
            mutableSelf->addMapping(key, contextType);
      }

      return contextType;
   }

   return Type();
}

Type GenericEnvironment::mapTypeIntoContext(
   Type type,
   LookupConformanceFn lookupConformance) const {
   assert(!type->hasOpenedExistential() &&
          "Opened existentials are special and so are you");

   Type result = type.subst(QueryInterfaceTypeSubstitutions(this),
                            lookupConformance,
                            SubstFlags::AllowLoweredTypes);
   assert((!result->hasTypeParameter() || result->hasError()) &&
          "not fully substituted");
   return result;

}

Type GenericEnvironment::mapTypeIntoContext(Type type) const {
   auto sig = getGenericSignature();
   return mapTypeIntoContext(type, LookUpConformanceInSignature(sig.getPointer()));
}

Type GenericEnvironment::mapTypeIntoContext(GenericTypeParamType *type) const {
   auto self = const_cast<GenericEnvironment *>(this);
   Type result = QueryInterfaceTypeSubstitutions(self)(type);
   if (!result)
      return ErrorType::get(type);
   return result;
}

GenericTypeParamType *GenericEnvironment::getSugaredType(
   GenericTypeParamType *type) const {
   for (auto *sugaredType : getGenericParams())
      if (sugaredType->isEqual(type))
         return sugaredType;

   llvm_unreachable("missing generic parameter");
}

Type GenericEnvironment::getSugaredType(Type type) const {
   if (!type->hasTypeParameter())
      return type;

   return type.transform([this](Type Ty) -> Type {
      if (auto GP = dyn_cast<GenericTypeParamType>(Ty.getPointer())) {
         return Type(getSugaredType(GP));
      }
      return Ty;
   });
}

SubstitutionMap GenericEnvironment::getForwardingSubstitutionMap() const {
   auto genericSig = getGenericSignature();
   return SubstitutionMap::get(genericSig,
                               QueryInterfaceTypeSubstitutions(this),
                               MakeAbstractConformanceForGenericType());
}

std::pair <Type, InterfaceConformanceRef>
GenericEnvironment::mapConformanceRefIntoContext(GenericEnvironment *genericEnv,
                                                 Type conformingType,
                                                 InterfaceConformanceRef conformance) {
   if (!genericEnv)
      return {conformingType, conformance};

   return genericEnv->mapConformanceRefIntoContext(conformingType, conformance);
}

std::pair <Type, InterfaceConformanceRef>
GenericEnvironment::mapConformanceRefIntoContext(
   Type conformingInterfaceType,
   InterfaceConformanceRef conformance) const {
   auto contextConformance = conformance.subst(conformingInterfaceType,
                                               QueryInterfaceTypeSubstitutions(this),
                                               LookUpConformanceInSignature(getGenericSignature().getPointer()));

   auto contextType = mapTypeIntoContext(conformingInterfaceType);
   return {contextType, contextConformance};
}

} // polar