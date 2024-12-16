//===--- PILBuiltinVisitor.h ------------------------------------*- C++ -*-===//
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
/// This file contains PILBuiltinVisitor, a visitor for visiting all possible
/// builtins and llvm intrinsics able to be used by BuiltinInst.
///
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_PILBUILTINVISITOR_H
#define POLARPHP_PIL_PILBUILTINVISITOR_H

#include "polarphp/pil/lang/PILInstruction.h"
#include <type_traits>

namespace polar {

template <typename ImplClass, typename ValueRetTy = void>
class PILBuiltinVisitor {
public:
   ImplClass &asImpl() { return static_cast<ImplClass &>(*this); }

   /// Perform any required pre-processing before visiting.
   ///
   /// Sub-classes can override this method to provide custom pre-processing.
   void beforeVisit(BuiltinInst *BI) {}

   ValueRetTy visit(BuiltinInst *BI) {
      asImpl().beforeVisit(BI);

      if (auto BuiltinKind = BI->getBuiltinKind()) {
         switch (BuiltinKind.getValue()) {
            // BUILTIN_TYPE_CHECKER_OPERATION does not live past the type checker.
#define BUILTIN_TYPE_CHECKER_OPERATION(ID, NAME)                               \
  case BuiltinValueKind::ID:                                                   \
    llvm_unreachable("Unexpected type checker operation seen in PIL!");

#define BUILTIN(ID, NAME, ATTRS)                                               \
  case BuiltinValueKind::ID:                                                   \
    return asImpl().visit##ID(BI, ATTRS);
#include "polarphp/ast/BuiltinsDef.h"
            case BuiltinValueKind::None:
               llvm_unreachable("None case");
         }
         llvm_unreachable("Not all cases handled?!");
      }

      if (auto IntrinsicID = BI->getIntrinsicID()) {
         return asImpl().visitLLVMIntrinsic(BI, IntrinsicID.getValue());
      }
      llvm_unreachable("Not all cases handled?!");
   }

   ValueRetTy visitLLVMIntrinsic(BuiltinInst *BI, llvm::Intrinsic::ID ID) {
      return ValueRetTy();
   }

   ValueRetTy visitBuiltinValueKind(BuiltinInst *BI, BuiltinValueKind Kind,
                                    StringRef Attrs) {
      return ValueRetTy();
   }

#define BUILTIN(ID, NAME, ATTRS)                                               \
  ValueRetTy visit##ID(BuiltinInst *BI, StringRef) {                           \
    return asImpl().visitBuiltinValueKind(BI, BuiltinValueKind::ID, ATTRS);    \
  }
#include "polarphp/ast/BuiltinsDef.h"
};

} // end polar namespace

#endif
