//===--- TypeRefs.def - Swift Type References -------------------*- C++ -*-===//
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
// This file defines Swift Type Reference Kinds.
//
//===----------------------------------------------------------------------===//

// TYPEREF(Id, Parent)

TYPEREF(Builtin, TypeRef)
TYPEREF(Nominal, TypeRef)
TYPEREF(BoundGeneric, TypeRef)
TYPEREF(Tuple, TypeRef)
TYPEREF(Function, TypeRef)
TYPEREF(InterfaceComposition, TypeRef)
TYPEREF(Metatype, TypeRef)
TYPEREF(ExistentialMetatype, TypeRef)
TYPEREF(GenericTypeParameter, TypeRef)
TYPEREF(DependentMember, TypeRef)
TYPEREF(ForeignClass, TypeRef)
TYPEREF(ObjCClass, TypeRef)
TYPEREF(ObjCInterface, TypeRef)
TYPEREF(Opaque, TypeRef)
#define REF_STORAGE(Name, ...) \
  TYPEREF(Name##Storage, TypeRef)
#include "polarphp/ast/ReferenceStorageDef.h"
TYPEREF(PILBox, TypeRef)

#undef TYPEREF
