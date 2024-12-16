//===--- Outlining.h - Value operation outlining ----------------*- C++ -*-===//
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
// This file defines interfaces for outlining value witnesses.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_IRGEN_INTERNAL_OUTLINING_H
#define POLARPHP_IRGEN_INTERNAL_OUTLINING_H

#include "polarphp/basic/LLVM.h"
#include "llvm/ADT/MapVector.h"

namespace llvm {
class Value;
class Type;
}

namespace polar {

class CanGenericSignature;
class CanType;
enum IsInitialization_t : bool;
enum IsTake_t : bool;
class PILType;

namespace irgen {
class Address;
class Explosion;
class IRGenFunction;
class IRGenModule;
class LocalTypeDataKey;
class TypeInfo;

/// A helper class for emitting outlined value operations.
///
/// The use-pattern for this class is:
///   - construct it
///   - collect all the metadata that will be required in order to perform
///     the value operations
///   - emit the call to the outlined copy/destroy helper
class OutliningMetadataCollector {
public:
   IRGenFunction &IGF;
private:
   llvm::MapVector<LocalTypeDataKey, llvm::Value *> Values;
   friend class IRGenModule;

public:
   OutliningMetadataCollector(IRGenFunction &IGF) : IGF(IGF) {}

   void collectFormalTypeMetadata(CanType type);
   void collectTypeMetadataForLayout(PILType type);

   void emitCallToOutlinedCopy(Address dest, Address src,
                               PILType T, const TypeInfo &ti,
                               IsInitialization_t isInit, IsTake_t isTake) const;
   void emitCallToOutlinedDestroy(Address addr, PILType T,
                                  const TypeInfo &ti) const;

private:
   void addMetadataArguments(SmallVectorImpl<llvm::Value *> &args) const ;
   void addMetadataParameterTypes(SmallVectorImpl<llvm::Type *> &paramTys) const;
   void bindMetadataParameters(IRGenFunction &helperIGF,
                               Explosion &params) const;
};

std::pair<CanType, CanGenericSignature>
getTypeAndGenericSignatureForManglingOutlineFunction(PILType type);


} // irgen
} // polar

#endif // POLARPHP_IRGEN_INTERNAL_OUTLINING_H
