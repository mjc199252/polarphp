//===--- DerivedConformanceError.cpp - Derived Error ----------------------===//
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
//  This file implements implicit derivation of the Error
//  protocol.
//
//===----------------------------------------------------------------------===//

#include "polarphp/sema/internal/TypeChecker.h"
#include "polarphp/sema/internal/DerivedConformances.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/Stmt.h"
#include "polarphp/ast/Expr.h"
#include "polarphp/ast/Module.h"

using namespace polar;

static std::pair<BraceStmt *, bool>
deriveBodyBridgedNSError_enum_nsErrorDomain(AbstractFunctionDecl *domainDecl,
                                            void *) {
   // enum SomeEnum {
   //   @derived
   //   static var _nsErrorDomain: String {
   //     return String(reflecting: self)
   //   }
   // }

   auto M = domainDecl->getParentModule();
   auto &C = M->getAstContext();
   auto self = domainDecl->getImplicitSelfDecl();

   auto selfRef = new (C) DeclRefExpr(self, DeclNameLoc(), /*implicit*/ true);
   auto stringType = TypeExpr::createForDecl(SourceLoc(), C.getStringDecl(),
                                             domainDecl, /*implicit*/ true);
   auto initReflectingCall =
      CallExpr::createImplicit(C, stringType,
                               { selfRef }, { C.getIdentifier("reflecting") });
   auto ret =
      new (C) ReturnStmt(SourceLoc(), initReflectingCall, /*implicit*/ true);

   auto body = BraceStmt::create(C, SourceLoc(), AstNode(ret), SourceLoc());
   return { body, /*isTypeChecked=*/false };
}
//
//static std::pair<BraceStmt *, bool>
//deriveBodyBridgedNSError_printAsObjCEnum_nsErrorDomain(
//   AbstractFunctionDecl *domainDecl, void *) {
//   // enum SomeEnum {
//   //   @derived
//   //   static var _nsErrorDomain: String {
//   //     return "ModuleName.SomeEnum"
//   //   }
//   // }
//
//   auto M = domainDecl->getParentModule();
//   auto &C = M->getAstContext();
//   auto TC = domainDecl->getInnermostTypeContext();
//   auto ED = TC->getSelfEnumDecl();
//   // @todo
////   StringRef value(C.AllocateCopy(getErrorDomainStringForObjC(ED)));
//
////   auto string = new (C) StringLiteralExpr(value, SourceRange(), /*implicit*/ true);
////   auto ret = new (C) ReturnStmt(SourceLoc(), string, /*implicit*/ true);
//   auto body = BraceStmt::create(C, SourceLoc(),
//                                 AstNode(ret),
//                                 SourceLoc());
//   return { body, /*isTypeChecked=*/false };
//}
//
//static ValueDecl *
//deriveBridgedNSError_enum_nsErrorDomain(
//   DerivedConformance &derived,
//   std::pair<BraceStmt *, bool> (*synthesizer)(AbstractFunctionDecl *, void*)) {
//   // enum SomeEnum {
//   //   @derived
//   //   static var _nsErrorDomain: String {
//   //     ...
//   //   }
//   // }
//
//   auto stringTy = derived.Context.getStringDecl()->getDeclaredType();
//
//   // Define the property.
//   VarDecl *propDecl;
//   PatternBindingDecl *pbDecl;
//   std::tie(propDecl, pbDecl) = derived.declareDerivedProperty(
//      derived.Context.Id_nsErrorDomain, stringTy, stringTy, /*isStatic=*/true,
//      /*isFinal=*/true);
//
//   // Define the getter.
//   auto getterDecl = derived.addGetterToReadOnlyDerivedProperty(
//      propDecl, stringTy);
//   getterDecl->setBodySynthesizer(synthesizer);
//
//   derived.addMembersToConformanceContext({propDecl, pbDecl});
//
//   return propDecl;
//}

//ValueDecl *DerivedConformance::deriveBridgedNSError(ValueDecl *requirement) {
//   if (!isa<EnumDecl>(Nominal))
//      return nullptr;
//
//   if (requirement->getBaseName() == Context.Id_nsErrorDomain) {
//      auto synthesizer = deriveBodyBridgedNSError_enum_nsErrorDomain;
//
//      auto scope = Nominal->getFormalAccessScope(Nominal->getModuleScopeContext());
//      if (scope.isPublic() || scope.isInternal())
//         // PrintAsObjC may print this domain, so we should make sure we use the
//         // same string it will.
//         synthesizer = deriveBodyBridgedNSError_printAsObjCEnum_nsErrorDomain;
//
//      return deriveBridgedNSError_enum_nsErrorDomain(*this, synthesizer);
//   }
//
//   Context.Diags.diagnose(requirement->getLoc(),
//                          diag::broken_errortype_requirement);
//   return nullptr;
//}
