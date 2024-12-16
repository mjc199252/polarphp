//===--- PILGenProlog.cpp - Function prologue emission --------------------===//
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

#include "polarphp/pil/gen/PILGenFunction.h"
#include "polarphp/pil/gen/ManagedValue.h"
#include "polarphp/pil/gen/Scope.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/ast/GenericEnvironment.h"
#include "polarphp/ast/ParameterList.h"

using namespace polar;
using namespace lowering;

PILValue PILGenFunction::emitSelfDecl(VarDecl *selfDecl) {
   // Emit the implicit 'self' argument.
   PILType selfType = getLoweredLoadableType(selfDecl->getType());
   PILValue selfValue = F.begin()->createFunctionArgument(selfType, selfDecl);
   VarLocs[selfDecl] = VarLoc::get(selfValue);
   PILLocation PrologueLoc(selfDecl);
   PrologueLoc.markAsPrologue();
   uint16_t ArgNo = 1; // Hardcoded for destructors.
   B.createDebugValue(PrologueLoc, selfValue,
                      PILDebugVariable(selfDecl->isLet(), ArgNo));
   return selfValue;
}

namespace {

/// Cleanup that writes back to an inout argument on function exit.
class CleanupWriteBackToInOut : public Cleanup {
   VarDecl *var;
   PILValue inoutAddr;

public:
   CleanupWriteBackToInOut(VarDecl *var, PILValue inoutAddr)
      : var(var), inoutAddr(inoutAddr) {}

   void emit(PILGenFunction &SGF, CleanupLocation l,
             ForUnwind_t forUnwind) override {
      // Assign from the local variable to the inout address with an
      // 'autogenerated' copyaddr.
      l.markAutoGenerated();
      SGF.B.createCopyAddr(l, SGF.VarLocs[var].value, inoutAddr,
                           IsNotTake, IsNotInitialization);
   }
};
} // end anonymous namespace


namespace {
class StrongReleaseCleanup : public Cleanup {
   PILValue box;
public:
   StrongReleaseCleanup(PILValue box) : box(box) {}
   void emit(PILGenFunction &SGF, CleanupLocation l,
             ForUnwind_t forUnwind) override {
      SGF.B.emitDestroyValueOperation(l, box);
   }

   void dump(PILGenFunction &) const override {
#ifndef NDEBUG
      llvm::errs() << "DeallocateValueBuffer\n"
                   << "State: " << getState() << "box: " << box << "\n";
#endif
   }
};
} // end anonymous namespace


namespace {
class EmitBBArguments : public CanTypeVisitor<EmitBBArguments,
   /*RetTy*/ ManagedValue>
{
public:
   PILGenFunction &SGF;
   PILBasicBlock *parent;
   PILLocation loc;
   CanPILFunctionType fnTy;
   ArrayRef<PILParameterInfo> &parameters;

   EmitBBArguments(PILGenFunction &sgf, PILBasicBlock *parent, PILLocation l,
                   CanPILFunctionType fnTy,
                   ArrayRef<PILParameterInfo> &parameters)
      : SGF(sgf), parent(parent), loc(l), fnTy(fnTy), parameters(parameters) {}

   ManagedValue visitType(CanType t) {
      return visitType(t, /*isInOut=*/false);
   }

   ManagedValue visitType(CanType t, bool isInOut) {
      // The calling convention always uses minimal resilience expansion but
      // inside the function we lower/expand types in context of the current
      // function.
      auto argType = SGF.SGM.Types.getLoweredType(t, SGF.getTypeExpansionContext());
      auto argTypeConv =
         SGF.SGM.Types.getLoweredType(t, TypeExpansionContext::minimal());
      argType = argType.getCategoryType(argTypeConv.getCategory());

      if (isInOut)
         argType = PILType::getPrimitiveAddressType(argType.getAstType());

      // Pop the next parameter info.
      auto parameterInfo = parameters.front();
      parameters = parameters.slice(1);

      auto paramType =
         SGF.F.mapTypeIntoContext(SGF.getPILType(parameterInfo, fnTy));
      ManagedValue mv = SGF.B.createInputFunctionArgument(
         paramType, loc.getAsAstNode<ValueDecl>());

      if (argType != paramType) {
         // This is a hack to deal with the fact that Self.Type comes in as a
         // static metatype, but we have to downcast it to a dynamic Self
         // metatype to get the right semantics.
         assert(
            cast<DynamicSelfType>(
               argType.castTo<MetatypeType>().getInstanceType())
               .getSelfType()
            == paramType.castTo<MetatypeType>().getInstanceType());
         mv = SGF.B.createUncheckedBitCast(loc, mv, argType);
      }

      if (isInOut)
         return mv;

      // This can happen if the value is resilient in the calling convention
      // but not resilient locally.
      if (argType.isLoadable(SGF.F) && argType.isAddress()) {
         if (mv.isPlusOne(SGF))
            mv = SGF.B.createLoadTake(loc, mv);
         else
            mv = SGF.B.createLoadBorrow(loc, mv);
      }

      // If the value is a (possibly optional) ObjC block passed into the entry
      // point of the function, then copy it so we can treat the value reliably
      // as a heap object. Escape analysis can eliminate this copy if it's
      // unneeded during optimization.
      CanType objectType = t;
      if (auto theObjTy = t.getOptionalObjectType())
         objectType = theObjTy;
      if (isa<FunctionType>(objectType) &&
          cast<FunctionType>(objectType)->getRepresentation()
          == FunctionType::Representation::Block) {
         PILValue blockCopy = SGF.B.createCopyBlock(loc, mv.getValue());
         mv = SGF.emitManagedRValueWithCleanup(blockCopy);
      }
      return mv;
   }

   ManagedValue visitTupleType(CanTupleType t) {
      SmallVector<ManagedValue, 4> elements;

      auto &tl = SGF.SGM.Types.getTypeLowering(t, SGF.getTypeExpansionContext());
      bool canBeGuaranteed = tl.isLoadable();

      // Collect the exploded elements.
      for (auto fieldType : t.getElementTypes()) {
         auto elt = visit(fieldType);
         // If we can't borrow one of the elements as a guaranteed parameter, then
         // we have to +1 the tuple.
         if (elt.hasCleanup())
            canBeGuaranteed = false;
         elements.push_back(elt);
      }

      if (tl.isLoadable() || !SGF.silConv.useLoweredAddresses()) {
         SmallVector<PILValue, 4> elementValues;
         if (canBeGuaranteed) {
            // If all of the elements were guaranteed, we can form a guaranteed tuple.
            for (auto element : elements)
               elementValues.push_back(element.getUnmanagedValue());
         } else {
            // Otherwise, we need to move or copy values into a +1 tuple.
            for (auto element : elements) {
               PILValue value = element.hasCleanup()
                                ? element.forward(SGF)
                                : element.copyUnmanaged(SGF, loc).forward(SGF);
               elementValues.push_back(value);
            }
         }
         auto tupleValue = SGF.B.createTuple(loc, tl.getLoweredType(),
                                             elementValues);
         return canBeGuaranteed
                ? ManagedValue::forUnmanaged(tupleValue)
                : SGF.emitManagedRValueWithCleanup(tupleValue);
      } else {
         // If the type is address-only, we need to move or copy the elements into
         // a tuple in memory.
         // TODO: It would be a bit more efficient to use a preallocated buffer
         // in this case.
         auto buffer = SGF.emitTemporaryAllocation(loc, tl.getLoweredType());
         for (auto i : indices(elements)) {
            auto element = elements[i];
            auto elementBuffer = SGF.B.createTupleElementAddr(loc, buffer,
                                                              i, element.getType().getAddressType());
            if (element.hasCleanup())
               element.forwardInto(SGF, loc, elementBuffer);
            else
               element.copyInto(SGF, loc, elementBuffer);
         }
         return SGF.emitManagedRValueWithCleanup(buffer);
      }
   }
};
} // end anonymous namespace


namespace {

/// A helper for creating PILArguments and binding variables to the argument
/// names.
struct ArgumentInitHelper {
   PILGenFunction &SGF;
   PILFunction &f;
   PILGenBuilder &initB;

   /// An ArrayRef that we use in our PILParameterList queue. Parameters are
   /// sliced off of the front as they're emitted.
   ArrayRef<PILParameterInfo> parameters;
   uint16_t ArgNo = 0;

   ArgumentInitHelper(PILGenFunction &SGF, PILFunction &f)
      : SGF(SGF), f(f), initB(SGF.B),
        parameters(
           f.getLoweredFunctionTypeInContext(SGF.B.getTypeExpansionContext())
              ->getParameters()) {}

   unsigned getNumArgs() const { return ArgNo; }

   ManagedValue makeArgument(Type ty, bool isInOut, PILBasicBlock *parent,
                             PILLocation l) {
      assert(ty && "no type?!");

      // Create an RValue by emitting destructured arguments into a basic block.
      CanType canTy = ty->getCanonicalType();
      EmitBBArguments argEmitter(SGF, parent, l,
                                 f.getLoweredFunctionType(), parameters);

      // Note: inouts of tuples are not exploded, so we bypass visit().
      if (isInOut)
         return argEmitter.visitType(canTy, /*isInOut=*/true);
      return argEmitter.visit(canTy);
   }

   /// Create a PILArgument and store its value into the given Initialization,
   /// if not null.
   void makeArgumentIntoBinding(Type ty, PILBasicBlock *parent, ParamDecl *pd) {
      PILLocation loc(pd);
      loc.markAsPrologue();

      ManagedValue argrv = makeArgument(ty, pd->isInOut(), parent, loc);

      if (pd->isInOut()) {
         assert(argrv.getType().isAddress() && "expected inout to be address");
      } else {
         assert(pd->isImmutable() && "expected parameter to be immutable!");
         // If the variable is immutable, we can bind the value as is.
         // Leave the cleanup on the argument, if any, in place to consume the
         // argument if we're responsible for it.
      }
      SGF.VarLocs[pd] = PILGenFunction::VarLoc::get(argrv.getValue());
      PILValue value = argrv.getValue();
      PILDebugVariable varinfo(pd->isImmutable(), ArgNo);
      if (!argrv.getType().isAddress()) {
         SGF.B.createDebugValue(loc, value, varinfo);
      } else {
         if (auto AllocStack = dyn_cast<AllocStackInst>(value))
            AllocStack->setArgNo(ArgNo);
         else
            SGF.B.createDebugValueAddr(loc, value, varinfo);
      }
   }

   void emitParam(ParamDecl *PD) {
      auto type = PD->getType();

      assert(type->isMaterializable());

      ++ArgNo;
      if (PD->hasName()) {
         makeArgumentIntoBinding(type, &*f.begin(), PD);
         return;
      }

      emitAnonymousParam(type, PD, PD);
   }

   void emitAnonymousParam(Type type, PILLocation paramLoc, ParamDecl *PD) {
      // A value bound to _ is unused and can be immediately released.
      Scope discardScope(SGF.Cleanups, CleanupLocation(PD));

      // Manage the parameter.
      auto argrv = makeArgument(type, PD->isInOut(), &*f.begin(), paramLoc);

      // Emit debug information for the argument.
      PILLocation loc(PD);
      loc.markAsPrologue();
      if (argrv.getType().isAddress())
         SGF.B.createDebugValueAddr(loc, argrv.getValue(),
                                    PILDebugVariable(PD->isLet(), ArgNo));
      else
         SGF.B.createDebugValue(loc, argrv.getValue(),
                                PILDebugVariable(PD->isLet(), ArgNo));
   }
};
} // end anonymous namespace


static void makeArgument(Type ty, ParamDecl *decl,
                         SmallVectorImpl<PILValue> &args, PILGenFunction &SGF) {
   assert(ty && "no type?!");

   // Destructure tuple value arguments.
   if (TupleType *tupleTy = decl->isInOut() ? nullptr : ty->getAs<TupleType>()) {
      for (auto fieldType : tupleTy->getElementTypes())
         makeArgument(fieldType, decl, args, SGF);
   } else {
      auto loweredTy = SGF.getLoweredTypeForFunctionArgument(ty);
      if (decl->isInOut())
         loweredTy = PILType::getPrimitiveAddressType(loweredTy.getAstType());
      auto arg = SGF.F.begin()->createFunctionArgument(loweredTy, decl);
      args.push_back(arg);
   }
}


void PILGenFunction::bindParameterForForwarding(ParamDecl *param,
                                                SmallVectorImpl<PILValue> &parameters) {
   makeArgument(param->getType(), param, parameters, *this);
}

void PILGenFunction::bindParametersForForwarding(const ParameterList *params,
                                                 SmallVectorImpl<PILValue> &parameters) {
   for (auto param : *params)
      bindParameterForForwarding(param, parameters);
}

static void emitCaptureArguments(PILGenFunction &SGF,
                                 GenericSignature origGenericSig,
                                 CapturedValue capture,
                                 uint16_t ArgNo) {

   auto *VD = cast<VarDecl>(capture.getDecl());
   PILLocation Loc(VD);
   Loc.markAsPrologue();

   // Local function to get the captured variable type within the capturing
   // context.
   auto getVarTypeInCaptureContext = [&]() -> Type {
      auto interfaceType = VD->getInterfaceType()->getCanonicalType(
         origGenericSig);
      return SGF.F.mapTypeIntoContext(interfaceType);
   };

   auto expansion = SGF.getTypeExpansionContext();
   switch (SGF.SGM.Types.getDeclCaptureKind(capture, expansion)) {
      case CaptureKind::Constant: {
         auto type = getVarTypeInCaptureContext();
         auto &lowering = SGF.getTypeLowering(type);
         // Constant decls are captured by value.
         PILType ty = lowering.getLoweredType();
         PILValue val = SGF.F.begin()->createFunctionArgument(ty, VD);

         bool NeedToDestroyValueAtExit = false;

         // If the original variable was settable, then Sema will have treated the
         // VarDecl as an lvalue, even in the closure's use.  As such, we need to
         // allow formation of the address for this captured value.  Create a
         // temporary within the closure to provide this address.
         if (VD->isSettable(VD->getDeclContext())) {
            auto addr = SGF.emitTemporaryAllocation(VD, ty);
            // We have created a copy that needs to be destroyed.
            val = SGF.B.emitCopyValueOperation(Loc, val);
            NeedToDestroyValueAtExit = true;
            lowering.emitStore(SGF.B, VD, val, addr, StoreOwnershipQualifier::Init);
            val = addr;
         }

         SGF.VarLocs[VD] = PILGenFunction::VarLoc::get(val);
         if (auto *AllocStack = dyn_cast<AllocStackInst>(val))
            AllocStack->setArgNo(ArgNo);
         else {
            PILDebugVariable DbgVar(/*Constant*/ true, ArgNo);
            SGF.B.createDebugValue(Loc, val, DbgVar);
         }

         // TODO: Closure contexts should always be guaranteed.
         if (NeedToDestroyValueAtExit && !lowering.isTrivial())
            SGF.enterDestroyCleanup(val);
         break;
      }

      case CaptureKind::Box: {
         // LValues are captured as a retained @box that owns
         // the captured value.
         auto type = getVarTypeInCaptureContext();
         // Get the content for the box in the minimal  resilience domain because we
         // are declaring a type.
         auto boxTy = SGF.SGM.Types.getContextBoxTypeForCapture(
            VD,
            SGF.SGM.Types.getLoweredRValueType(TypeExpansionContext::minimal(),
                                               type),
            SGF.F.getGenericEnvironment(), /*mutable*/ true);
         PILValue box = SGF.F.begin()->createFunctionArgument(
            PILType::getPrimitiveObjectType(boxTy), VD);
         PILValue addr = SGF.B.createProjectBox(VD, box, 0);
         SGF.VarLocs[VD] = PILGenFunction::VarLoc::get(addr, box);
         PILDebugVariable DbgVar(/*Constant*/ false, ArgNo);
         SGF.B.createDebugValueAddr(Loc, addr, DbgVar);
         break;
      }
      case CaptureKind::StorageAddress: {
         // Non-escaping stored decls are captured as the address of the value.
         auto type = getVarTypeInCaptureContext();
         PILType ty = SGF.getLoweredType(type).getAddressType();
         PILValue addr = SGF.F.begin()->createFunctionArgument(ty, VD);
         SGF.VarLocs[VD] = PILGenFunction::VarLoc::get(addr);
         PILDebugVariable DbgVar(/*Constant*/ true, ArgNo);
         SGF.B.createDebugValueAddr(Loc, addr, DbgVar);
         break;
      }
   }
}

void PILGenFunction::emitProlog(CaptureInfo captureInfo,
                                ParameterList *paramList,
                                ParamDecl *selfParam,
                                DeclContext *DC,
                                Type resultType,
                                bool throws,
                                SourceLoc throwsLoc) {
   uint16_t ArgNo = emitProlog(paramList, selfParam, resultType,
                               DC, throws, throwsLoc);

   // Emit an unreachable instruction if a parameter type is
   // uninhabited
   if (paramList) {
      for (auto *param : *paramList) {
         if (param->getType()->isStructurallyUninhabited()) {
            PILLocation unreachableLoc(param);
            unreachableLoc.markAsPrologue();
            B.createUnreachable(unreachableLoc);
            break;
         }
      }
   }

   // Emit the capture argument variables. These are placed last because they
   // become the first curry level of the PIL function.
   assert((captureInfo.hasBeenComputed() ||
           !TypeConverter::canCaptureFromParent(DC)) &&
          "can't emit prolog of function with uncomputed captures");
   for (auto capture : captureInfo.getCaptures()) {
      if (capture.isDynamicSelfMetadata()) {
         auto selfMetatype = MetatypeType::get(
            captureInfo.getDynamicSelfType());
         PILType ty = getLoweredType(selfMetatype);
         PILValue val = F.begin()->createFunctionArgument(ty);
         (void) val;

         continue;
      }

      if (capture.isOpaqueValue()) {
         OpaqueValueExpr *opaqueValue = capture.getOpaqueValue();
         Type type = opaqueValue->getType()->mapTypeOutOfContext();
         type = F.mapTypeIntoContext(type);
         auto &lowering = getTypeLowering(type);
         PILType ty = lowering.getLoweredType();
         PILValue val = F.begin()->createFunctionArgument(ty);
         OpaqueValues[opaqueValue] = ManagedValue::forUnmanaged(val);

         // Opaque values are always passed 'owned', so add a clean up if needed.
         if (!lowering.isTrivial())
            enterDestroyCleanup(val);

         continue;
      }

      emitCaptureArguments(*this, DC->getGenericSignatureOfContext(),
                           capture, ++ArgNo);
   }
}

static void emitIndirectResultParameters(PILGenFunction &SGF, Type resultType,
                                         DeclContext *DC) {
   // Expand tuples.
   if (auto tupleType = resultType->getAs<TupleType>()) {
      for (auto eltType : tupleType->getElementTypes()) {
         emitIndirectResultParameters(SGF, eltType, DC);
      }
      return;
   }

   // If the return type is address-only, emit the indirect return argument.

   // The calling convention always uses minimal resilience expansion.
   auto &resultTI =
      SGF.SGM.Types.getTypeLowering(DC->mapTypeIntoContext(resultType),
                                    SGF.getTypeExpansionContext());
   auto &resultTIConv = SGF.SGM.Types.getTypeLowering(
      DC->mapTypeIntoContext(resultType), TypeExpansionContext::minimal());

   if (!PILModuleConventions::isReturnedIndirectlyInPIL(
      resultTIConv.getLoweredType(), SGF.SGM.M)) {
      return;
   }
   auto &ctx = SGF.getAstContext();
   auto var = new (ctx) ParamDecl(SourceLoc(), SourceLoc(),
                                  ctx.getIdentifier("$return_value"), SourceLoc(),
                                  ctx.getIdentifier("$return_value"),
                                  DC);
   var->setSpecifier(ParamSpecifier::InOut);
   var->setInterfaceType(resultType);
   auto *arg = SGF.F.begin()->createFunctionArgument(
      resultTI.getLoweredType().getAddressType(), var);
   (void)arg;
}

uint16_t PILGenFunction::emitProlog(ParameterList *paramList,
                                    ParamDecl *selfParam,
                                    Type resultType,
                                    DeclContext *DC,
                                    bool throws,
                                    SourceLoc throwsLoc) {
   // Create the indirect result parameters.
   auto genericSig = DC->getGenericSignatureOfContext();
   resultType = resultType->getCanonicalType(genericSig);

   emitIndirectResultParameters(*this, resultType, DC);

   // Emit the argument variables in calling convention order.
   ArgumentInitHelper emitter(*this, F);

   // Add the PILArguments and use them to initialize the local argument
   // values.
   if (paramList)
      for (auto *param : *paramList)
         emitter.emitParam(param);
   if (selfParam)
      emitter.emitParam(selfParam);

   // Record the ArgNo of the artificial $error inout argument.
   unsigned ArgNo = emitter.getNumArgs();
   if (throws) {
      auto NativeErrorTy = PILType::getExceptionType(getAstContext());
      ManagedValue Undef = emitUndef(NativeErrorTy);
      PILDebugVariable DbgVar("$error", /*Constant*/ false, ++ArgNo);
      RegularLocation loc = RegularLocation::getAutoGeneratedLocation();
      if (throwsLoc.isValid())
         loc = throwsLoc;
      B.createDebugValue(loc, Undef.getValue(), DbgVar);
   }

   return ArgNo;
}