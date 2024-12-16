//===--- Builtins.def - Builtins Macro Metaprogramming Database -*- C++ -*-===//
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
// This file defines the database of builtin functions.
//
// BUILTIN(Id, Name, Attrs)
//   - Id is an identifier suitable for use in C++
//   - Name is a string literal for the name to which the builtin should be
//     bound in Swift
//   - Attrs specifies information about attributes of the function:
//     n -> readnone
//
//===----------------------------------------------------------------------===//

/// Cast operations have type T1 -> T2.
#ifndef BUILTIN_CAST_OPERATION
#define BUILTIN_CAST_OPERATION(Id, Name, Attrs) BUILTIN(Id, Name, Attrs)
#endif
BUILTIN_CAST_OPERATION(Trunc   , "trunc",    "n")
BUILTIN_CAST_OPERATION(ZExt    , "zext",     "n")
BUILTIN_CAST_OPERATION(SExt    , "sext",     "n")
BUILTIN_CAST_OPERATION(FPToUI  , "fptoui",   "n")
BUILTIN_CAST_OPERATION(FPToSI  , "fptosi",   "n")
BUILTIN_CAST_OPERATION(UIToFP  , "uitofp",   "n")
BUILTIN_CAST_OPERATION(SIToFP  , "sitofp",   "n")
BUILTIN_CAST_OPERATION(FPTrunc , "fptrunc",  "n")
BUILTIN_CAST_OPERATION(FPExt   , "fpext",    "n")
BUILTIN_CAST_OPERATION(PtrToInt, "ptrtoint", "n")
BUILTIN_CAST_OPERATION(IntToPtr, "inttoptr", "n")
BUILTIN_CAST_OPERATION(BitCast , "bitcast",  "n")

#undef BUILTIN_CAST_OPERATION

/// Cast-or-bitcast operations have type T1 -> T2.
/// T1 and T2 may be the same size, unlike the corresponding true casts.
#ifndef BUILTIN_CAST_OR_BITCAST_OPERATION
#define BUILTIN_CAST_OR_BITCAST_OPERATION(Id, Name, Attrs) BUILTIN(Id, Name, Attrs)
#endif
BUILTIN_CAST_OR_BITCAST_OPERATION(TruncOrBitCast, "truncOrBitCast", "n")
BUILTIN_CAST_OR_BITCAST_OPERATION(ZExtOrBitCast,  "zextOrBitCast",  "n")
BUILTIN_CAST_OR_BITCAST_OPERATION(SExtOrBitCast,  "sextOrBitCast",  "n")
#undef BUILTIN_CAST_OR_BITCAST_OPERATION

/// Binary operations have type (T,T) -> T.
///
/// We define two different sorts of operations varying when T is static,
/// specifically:
///
/// 1. Overloaded statically typed operations. E.x:
///
///       builtin "add_Vec4xInt32"(Vec4xInt32, Vec4xInt32) : Vec4xInt32.
///
/// 2. Polymorphic typed operations that are valid only in raw PIL. By the time
///    diagnostic constant propagation runs, these must have as its operand a
///    fully specialized type. If the builtin has a type that is not one of its
///    overloaded types, diagnostic constant propagation will emit a diagnostic
///    saying the builtin's type has not been fully resolved. Otherwise,
///    diagnostic constant propagation will transform the builtin to the
///    relevant static overloaded builtin form. E.x.:
///
///       builtin "add"(Self, Self) : Self // *error*
///
///         OR
///
///       builtin "generic_add"(Vec4xInt32, Vec4xInt32) : Vec4xInt32
///         ->
///       builtin "add_Vec4xInt32"(Vec4xInt32, Vec4xInt32) : Vec4xInt32
///
/// NOTE: If a polymorphic typed operation is not static by the time guaranteed
/// constant propagation runs, we emit a diagnostic to inform the user (who is
/// assumed to be an expert user) to tell them the value was unspecialized. The
/// typical way this specialization occurs today is via transparent inlining
/// since the transparent inliner devirtualizes and specializes as it goes. Of
/// course this means mandatory inlining must /always/ occur before diagnostic
/// constant propagation.
///
/// NOTE: Often times the builtin infrastructure wants to treat all
/// binary operation builtins generic or not the same way. To ensure
/// we support all use cases in the compiler, we do not declare the
/// operations as part of this builtin since often times this macro is
/// used to generic code. Instead, we stamp this out using the
/// overloaded_static, polymorphic, and all suffixed operations.
#ifndef BUILTIN_BINARY_OPERATION
#define BUILTIN_BINARY_OPERATION(Id, Name, Attrs) BUILTIN(Id, Name, Attrs)
#endif

#ifdef BUILTIN_BINARY_OPERATION_GENERIC_HELPER_STR
#error "Do not define BUILTIN_BINARY_OPERATION_GENERIC_HELPER_STR before including this .def file"
#endif

#define BUILTIN_BINARY_OPERATION_GENERIC_HELPER_STR(NAME) #NAME

#ifndef BUILTIN_BINARY_OPERATION_OVERLOADED_STATIC
#define BUILTIN_BINARY_OPERATION_OVERLOADED_STATIC(Id, Name, Attrs, Overload) \
  BUILTIN_BINARY_OPERATION(Id, Name, Attrs)
#endif

#ifndef BUILTIN_BINARY_OPERATION_POLYMORPHIC
#define BUILTIN_BINARY_OPERATION_POLYMORPHIC(Id, Name, Attrs) \
  BUILTIN_BINARY_OPERATION(Id, Name, Attrs)
#endif

// TODO: This needs a better name. We stringify generic_ in *_{OVERLOADED_STATIC,POLYMORPHIC}
#ifndef BUILTIN_BINARY_OPERATION_ALL
#define BUILTIN_BINARY_OPERATION_ALL(Id, Name, Attrs, Overload) \
  BUILTIN_BINARY_OPERATION_OVERLOADED_STATIC(Id, BUILTIN_BINARY_OPERATION_GENERIC_HELPER_STR(Name), Attrs, Overload) \
  BUILTIN_BINARY_OPERATION_POLYMORPHIC(Generic##Id, BUILTIN_BINARY_OPERATION_GENERIC_HELPER_STR(generic_##Name), Attrs)
#endif

// NOTE: Here we need our name field to be bare. We stringify them as
// appropriately in BUILTIN_BINARY_OPERATION_{OVERLOADED_STATIC,POLYMORPHIC}.
BUILTIN_BINARY_OPERATION_ALL(Add,     add,      "n", IntegerOrVector)
BUILTIN_BINARY_OPERATION_ALL(FAdd,    fadd,     "n", FloatOrVector)
BUILTIN_BINARY_OPERATION_ALL(And,     and,      "n", IntegerOrVector)
BUILTIN_BINARY_OPERATION_ALL(AShr,    ashr,     "n", IntegerOrVector)
BUILTIN_BINARY_OPERATION_ALL(LShr,    lshr,     "n", IntegerOrVector)
BUILTIN_BINARY_OPERATION_ALL(Or,      or,       "n", IntegerOrVector)
BUILTIN_BINARY_OPERATION_ALL(FDiv,    fdiv,     "n", FloatOrVector)
BUILTIN_BINARY_OPERATION_ALL(Mul,     mul,      "n", IntegerOrVector)
BUILTIN_BINARY_OPERATION_ALL(FMul,    fmul,     "n", FloatOrVector)
BUILTIN_BINARY_OPERATION_ALL(SDiv,    sdiv,     "n", IntegerOrVector)
BUILTIN_BINARY_OPERATION_ALL(ExactSDiv, sdiv_exact, "n", IntegerOrVector)
BUILTIN_BINARY_OPERATION_ALL(Shl,     shl,      "n", IntegerOrVector)
BUILTIN_BINARY_OPERATION_ALL(SRem,    srem,     "n", IntegerOrVector)
BUILTIN_BINARY_OPERATION_ALL(Sub,     sub,      "n", IntegerOrVector)
BUILTIN_BINARY_OPERATION_ALL(FSub,    fsub,     "n", FloatOrVector)
BUILTIN_BINARY_OPERATION_ALL(UDiv,    udiv,     "n", IntegerOrVector)
BUILTIN_BINARY_OPERATION_ALL(ExactUDiv, udiv_exact, "n", IntegerOrVector)
BUILTIN_BINARY_OPERATION_ALL(URem,    urem,     "n", Integer)
BUILTIN_BINARY_OPERATION_ALL(FRem,    frem,     "n", FloatOrVector)
BUILTIN_BINARY_OPERATION_ALL(Xor,     xor,      "n", IntegerOrVector)
BUILTIN_BINARY_OPERATION_OVERLOADED_STATIC(Expect, "int_expect", "n", Integer)
#undef BUILTIN_BINARY_OPERATION_ALL
#undef BUILTIN_BINARY_OPERATION_POLYMORPHIC
#undef BUILTIN_BINARY_OPERATION_OVERLOADED_STATIC
#undef BUILTIN_BINARY_OPERATION_GENERIC_HELPER_STR
#undef BUILTIN_BINARY_OPERATION

/// These builtins are analogous the similarly named llvm intrinsics. The
/// difference between the two is that these are not expected to overflow,
/// so we should produce a compile time error if we can statically prove
/// that they do.
#ifndef BUILTIN_BINARY_OPERATION_WITH_OVERFLOW
#define BUILTIN_BINARY_OPERATION_WITH_OVERFLOW(Id, Name, UncheckedID, Attrs, Overload) \
          BUILTIN(Id, Name, Attrs)
#endif
BUILTIN_BINARY_OPERATION_WITH_OVERFLOW(SAddOver,
                                       "sadd_with_overflow", Add, "n", Integer)
BUILTIN_BINARY_OPERATION_WITH_OVERFLOW(UAddOver,
                                       "uadd_with_overflow", Add, "n", Integer)
BUILTIN_BINARY_OPERATION_WITH_OVERFLOW(SSubOver,
                                       "ssub_with_overflow", Sub, "n", Integer)
BUILTIN_BINARY_OPERATION_WITH_OVERFLOW(USubOver,
                                       "usub_with_overflow", Sub, "n", Integer)
BUILTIN_BINARY_OPERATION_WITH_OVERFLOW(SMulOver,
                                       "smul_with_overflow", Mul, "n", Integer)
BUILTIN_BINARY_OPERATION_WITH_OVERFLOW(UMulOver,
                                       "umul_with_overflow", Mul, "n", Integer)
#undef BUILTIN_BINARY_OPERATION_WITH_OVERFLOW

/// Unary operations have type (T) -> T.
#ifndef BUILTIN_UNARY_OPERATION
#define BUILTIN_UNARY_OPERATION(Id, Name, Attrs, Overload) \
          BUILTIN(Id, Name, Attrs)
#endif

// "fneg" is a separate builtin because its LLVM representation is
// 'fsub -0.0, %x', but defining it in swift as
// 'func [prefix] -(x) { -0.0 - x }' would be infinitely recursive.
BUILTIN_UNARY_OPERATION(FNeg, "fneg", "n", FloatOrVector)

// Returns the argument and specifies that the value is not negative.
// It has only an effect if the argument is a load or call.
// TODO: consider printing a warning if it is not used on a load or call.
BUILTIN_UNARY_OPERATION(AssumeNonNegative, "assumeNonNegative", "n", Integer)
// It only works on i1.
BUILTIN_UNARY_OPERATION(AssumeTrue, "assume", "", Integer)

#undef BUILTIN_UNARY_OPERATION

// Binary predicates have type (T,T) -> i1 or (T, T) -> Vector<i1> for scalars
// and vectors, respectively.
#ifndef BUILTIN_BINARY_PREDICATE
#define BUILTIN_BINARY_PREDICATE(Id, Name, Attrs, Overload) \
          BUILTIN(Id, Name, Attrs)
#endif
BUILTIN_BINARY_PREDICATE(ICMP_EQ,  "cmp_eq",   "n", IntegerOrRawPointerOrVector)
BUILTIN_BINARY_PREDICATE(ICMP_NE,  "cmp_ne",   "n", IntegerOrRawPointerOrVector)
BUILTIN_BINARY_PREDICATE(ICMP_SLE, "cmp_sle",  "n", IntegerOrVector)
BUILTIN_BINARY_PREDICATE(ICMP_SLT, "cmp_slt",  "n", IntegerOrVector)
BUILTIN_BINARY_PREDICATE(ICMP_SGE, "cmp_sge",  "n", IntegerOrVector)
BUILTIN_BINARY_PREDICATE(ICMP_SGT, "cmp_sgt",  "n", IntegerOrVector)
BUILTIN_BINARY_PREDICATE(ICMP_ULE, "cmp_ule",  "n", IntegerOrRawPointerOrVector)
BUILTIN_BINARY_PREDICATE(ICMP_ULT, "cmp_ult",  "n", IntegerOrRawPointerOrVector)
BUILTIN_BINARY_PREDICATE(ICMP_UGE, "cmp_uge",  "n", IntegerOrRawPointerOrVector)
BUILTIN_BINARY_PREDICATE(ICMP_UGT, "cmp_ugt",  "n", IntegerOrRawPointerOrVector)
BUILTIN_BINARY_PREDICATE(FCMP_OEQ, "fcmp_oeq", "n", FloatOrVector)
BUILTIN_BINARY_PREDICATE(FCMP_OGT, "fcmp_ogt", "n", FloatOrVector)
BUILTIN_BINARY_PREDICATE(FCMP_OGE, "fcmp_oge", "n", FloatOrVector)
BUILTIN_BINARY_PREDICATE(FCMP_OLT, "fcmp_olt", "n", FloatOrVector)
BUILTIN_BINARY_PREDICATE(FCMP_OLE, "fcmp_ole", "n", FloatOrVector)
BUILTIN_BINARY_PREDICATE(FCMP_ONE, "fcmp_one", "n", FloatOrVector)
BUILTIN_BINARY_PREDICATE(FCMP_ORD, "fcmp_ord", "n", FloatOrVector)
BUILTIN_BINARY_PREDICATE(FCMP_UEQ, "fcmp_ueq", "n", FloatOrVector)
BUILTIN_BINARY_PREDICATE(FCMP_UGT, "fcmp_ugt", "n", FloatOrVector)
BUILTIN_BINARY_PREDICATE(FCMP_UGE, "fcmp_uge", "n", FloatOrVector)
BUILTIN_BINARY_PREDICATE(FCMP_ULT, "fcmp_ult", "n", FloatOrVector)
BUILTIN_BINARY_PREDICATE(FCMP_ULE, "fcmp_ule", "n", FloatOrVector)
BUILTIN_BINARY_PREDICATE(FCMP_UNE, "fcmp_une", "n", FloatOrVector)
BUILTIN_BINARY_PREDICATE(FCMP_UNO, "fcmp_uno", "n", FloatOrVector)
#undef BUILTIN_BINARY_PREDICATE

// BUILTIN_PIL_OPERATION - Operations that can be lowered to PIL instructions.
// These have various types.
// Since these operations will be lowered to PIL Instructions, we do not
// assign any attributes on them.
#ifndef BUILTIN_PIL_OPERATION
#define BUILTIN_PIL_OPERATION(Id, Name, Overload) BUILTIN(Id, Name, "")
#endif

/// retain: T -> ()
BUILTIN_PIL_OPERATION(Retain, "retain", Special)

/// release: T -> ()
BUILTIN_PIL_OPERATION(Release, "release", Special)

/// autorelease: T -> ()
BUILTIN_PIL_OPERATION(Autorelease, "autorelease", Special)

/// Load has type (Builtin.RawPointer) -> T
BUILTIN_PIL_OPERATION(Load, "load", Special)

/// LoadRaw has type (Builtin.RawPointer) -> T
/// This is a load of T from raw memory.
/// Its address does not adhere to strict aliasing.
BUILTIN_PIL_OPERATION(LoadRaw, "loadRaw", Special)

/// LoadInvariant has type (Builtin.RawPointer) -> T
/// This is a load of T from raw memory.
/// The load is marked as invariant.
BUILTIN_PIL_OPERATION(LoadInvariant, "loadInvariant", Special)

/// Take has type (Builtin.RawPointer) -> T
BUILTIN_PIL_OPERATION(Take, "take", Special)

/// Destroy has type (T.Type, Builtin.RawPointer) -> ()
BUILTIN_PIL_OPERATION(Destroy, "destroy", Special)

/// Assign has type (T, Builtin.RawPointer) -> ()
BUILTIN_PIL_OPERATION(Assign, "assign", Special)

/// Init has type (T, Builtin.RawPointer) -> ()
BUILTIN_PIL_OPERATION(Init, "initialize", Special)

/// CastToNativeObject has type (T) -> Builtin.NativeObject.
///
/// This builtin asserts if the underlying type /could/ be objc.
BUILTIN_PIL_OPERATION(CastToNativeObject, "castToNativeObject", Special)

/// UnsafeCastToNativeObject has type (T) -> Builtin.NativeObject.
///
/// This builtin does not check if the underlying type /could/ be objc.
BUILTIN_PIL_OPERATION(UnsafeCastToNativeObject, "unsafeCastToNativeObject", Special)

/// CastFromNativeObject has type (Builtin.NativeObject) -> T
BUILTIN_PIL_OPERATION(CastFromNativeObject, "castFromNativeObject", Special)

/// CastToBridgeObject has type (T, Builtin.Word) -> Builtin.BridgeObject.
/// It sets the BridgeObject to the bitwise OR of its operands.
/// It is assumed that
///
///   castReferenceFromBridgeObject(castToBridgeObject(ref, x)) === ref
///
/// regardless of what x is.
/// x thus must not have any bits set that would change the heap
/// object pointer value, nor may it have the native/ObjC discriminator bit set,
/// nor may it have any bits set if the first operand is an ObjC tagged pointer,
/// or else undefined behavior will ensue.
BUILTIN_PIL_OPERATION(CastToBridgeObject, "castToBridgeObject", Special)

/// ValueToBridgeObject has type (T) -> Builtin.BridgeObject.
/// It sets the BridgeObject to a tagged pointer representation holding its
//  operands by tagging and shifting the operand if needed.
///
/// valueToBridgeObject(x) === (x << _swift_abi_ObjCReservedLowBits) |
///     _swift_BridgeObject_TaggedPointerBits
///
/// x thus must not be using any high bits shifted away (via _swift_abi_ObjCReservedLowBits)
/// or the tag bits post-shift.
/// ARC operations on such tagged values are NOPs.
BUILTIN_PIL_OPERATION(ValueToBridgeObject, "valueToBridgeObject", Special)

/// CastReferenceFromBridgeObject has type (Builtin.BridgeObject) -> T.
/// It recovers the heap object reference by masking spare bits from the
/// BridgeObject.
BUILTIN_PIL_OPERATION(CastReferenceFromBridgeObject,
                      "castReferenceFromBridgeObject",
                      Special)

/// CastBitPatternFromBridgeObject has type (Builtin.BridgeObject) -> Builtin.Word.
/// It presents the raw bit pattern of the BridgeObject as
BUILTIN_PIL_OPERATION(CastBitPatternFromBridgeObject,
                      "castBitPatternFromBridgeObject",
                      Special)

/// ClassifyBridgeObject has type:
///      (Builtin.BridgeObject) -> (Builtin.Int1, Builtin.Int1).
/// It interprets the bits mangled into a bridge object, returning whether it is
/// an Objective-C object or tagged pointer representation.
BUILTIN_PIL_OPERATION(ClassifyBridgeObject, "classifyBridgeObject", Special)


/// BridgeToRawPointer has type (T) -> Builtin.RawPointer
BUILTIN_PIL_OPERATION(BridgeToRawPointer, "bridgeToRawPointer", Special)

/// BridgeFromRawPointer (Builtin.RawPointer) -> T
/// PILGen requires that T is a single retainable pointer.
/// Bridging to/from a raw pointer does not imply a retain.
BUILTIN_PIL_OPERATION(BridgeFromRawPointer, "bridgeFromRawPointer", Special)

/// castReference has type T -> U.
/// T and U must be convertible to AnyObject.
BUILTIN_PIL_OPERATION(CastReference, "castReference", Special)

/// reinterpretCast has type T -> U.
BUILTIN_PIL_OPERATION(ReinterpretCast, "reinterpretCast", Special)

/// addressof (inout T) -> Builtin.RawPointer
/// Returns a RawPointer pointing to a physical lvalue. The returned pointer is
/// only valid for the duration of the original binding.
BUILTIN_PIL_OPERATION(AddressOf, "addressof", Special)

/// addressOfBorrow (__shared T) -> Builtin.RawPointer
/// Returns a RawPointer pointing to a borrowed rvalue. The returned pointer is only
/// valid within the scope of the borrow.
BUILTIN_PIL_OPERATION(AddressOfBorrow, "addressOfBorrow", Special)

/// GepRaw(Builtin.RawPointer, Builtin.Word) -> Builtin.RawPointer
///
/// Adds index bytes to a base pointer.
BUILTIN_PIL_OPERATION(GepRaw, "gepRaw", Integer)

/// Gep (Builtin.RawPointer, Builtin.Word, T.Type) -> Builtin.RawPointer
///
/// Like the GepRaw-builtin, but multiplies the index by stride-of type 'T'.
BUILTIN_PIL_OPERATION(Gep, "gep", Integer)

/// getTailAddr(Builtin.RawPointer,
///             Builtin.Word, T.Type, E.Type) -> Builtin.RawPointer
///
/// Like the Gep-builtin, but rounds up the resulting address to a tail-
/// allocated element type 'E'.
BUILTIN_PIL_OPERATION(GetTailAddr, "getTailAddr", Integer)

/// performInstantaneousReadAccess(Builtin.RawPointer, T.Type) -> ()
/// Begin and then immediately end a read access to the given raw pointer,
/// which will be treated as an address of type 'T'.
BUILTIN_PIL_OPERATION(PerformInstantaneousReadAccess,
                      "performInstantaneousReadAccess", Special)

/// beginUnpairedModifyAccess(Builtin.RawPointer, Builtin.RawPointer,
///                           T.Type) -> ()
/// Begins but does not end a 'modify' access to the first raw pointer argument.
/// The second raw pointer must be a pointer to an UnsafeValueBuffer, which
/// will be used by the runtime to record the access. The lifetime of the
/// value buffer must be longer than that of the access itself. The accessed
/// address will be treated as having type 'T'.
BUILTIN_PIL_OPERATION(BeginUnpairedModifyAccess, "beginUnpairedModifyAccess",
                      Special)

/// endUnpairedAccess(Builtin.RawPointer) -> ()
/// Ends an in-progress unpaired access. The raw pointer argument must be
/// be a pointer to an UnsafeValueBuffer that records an in progress access.
BUILTIN_PIL_OPERATION(EndUnpairedAccess, "endUnpairedAccess", Special)

/// condfail(Int1) -> ()
/// Triggers a runtime failure if the condition is true.
/// This builtin is deprecated. Use condfail_message instead.
BUILTIN_PIL_OPERATION(LegacyCondFail, "condfail", Special)

/// fixLifetime(T) -> ()
/// Fixes the lifetime of any heap references in a value.
BUILTIN_PIL_OPERATION(FixLifetime, "fixLifetime", Special)

/// isUnique : <T> (inout T[?]) -> Int1
///
/// This builtin takes an inout object reference and returns a boolean. Passing
/// the reference inout forces the optimizer to preserve a retain distinct from
/// what's required to maintain lifetime for any of the reference's source-level
/// copies, because the called function is allowed to replace the reference,
/// thereby releasing the referent.
///
/// The kind of reference count checking that Builtin.isUnique performs depends
/// on the argument type:
///
/// - Native object types are directly checked by reading the
///   strong reference count:
///   (Builtin.NativeObject, known native class reference)
///
/// - Objective-C object types require an additional check that the
///   dynamic object type uses native swift reference counting:
///   (AnyObject, unknown class reference, class existential)
///
/// - Bridged object types allow the dynamic object type check to be
///   passed based on their pointer encoding:
///   (Builtin.BridgeObject)
///
/// Any of the above types may also be wrapped in an optional.
/// If the static argument type is optional, then a null check is also
/// performed.
///
/// Thus, isUnique only returns true for non-null, native swift object
/// references with a strong reference count of one.
BUILTIN_PIL_OPERATION(IsUnique, "isUnique", Special)

/// IsUnique_native : <T> (inout T[?]) -> Int1
///
/// These variants of isUnique implicitly cast to a non-null NativeObject before
/// checking uniqueness. This allows an object reference statically typed as
/// BridgeObject to be treated as a native object by the runtime.
BUILTIN_PIL_OPERATION(IsUnique_native, "isUnique_native", Special)

/// bindMemory : <T> (Builtin.RawPointer, Builtin.Word, T.Type) -> ()
BUILTIN_PIL_OPERATION(BindMemory, "bindMemory", Special)

/// allocWithTailElems_<n>(C.Type,
///                    Builtin.Word, E1.Type, ... , Builtin.Word, En.Type) -> C\
///
/// The integer suffix <n> specifies the number of tail-allocated arrays.
/// Each tail-allocated array adds a counter and an element meta-type parameter.
BUILTIN_PIL_OPERATION(AllocWithTailElems, "allocWithTailElems", Special)

/// projectTailElems : <C,E> (C) -> Builtin.RawPointer
///
/// Projects the first tail-allocated element of type E from a class C.
BUILTIN_PIL_OPERATION(ProjectTailElems, "projectTailElems", Special)

#undef BUILTIN_PIL_OPERATION

// BUILTIN_RUNTIME_CALL - A call into a runtime function.
// These functions accept a single argument of any type.
#ifndef BUILTIN_RUNTIME_CALL
#define BUILTIN_RUNTIME_CALL(Id, Name, Attrs) \
  BUILTIN(Id, Name, Attrs)
#endif

/// unexpectedError: Error -> ()
BUILTIN_RUNTIME_CALL(UnexpectedError, "unexpectedError", "")

/// errorInMain: Error -> ()
BUILTIN_RUNTIME_CALL(ErrorInMain, "errorInMain", "")

/// IsOptionalType : T.Type -> Bool
/// This builtin takes a metatype and returns true if the metatype's
/// nominal type is Optional.
BUILTIN_RUNTIME_CALL(IsOptionalType, "isOptional", "")

#undef BUILTIN_RUNTIME_CALL

// BUILTIN_MISC_OPERATION - Miscellaneous operations without a unifying class.
// These have various types.
#ifndef BUILTIN_MISC_OPERATION
#define BUILTIN_MISC_OPERATION(Id, Name, Attrs, Overload) \
          BUILTIN(Id, Name, Attrs)
#endif

/// condfail_message(Int1, RawPointer) -> ()
/// Triggers a runtime failure if the condition is true.
BUILTIN_MISC_OPERATION(CondFailMessage, "condfail_message", "", Special)

/// Sizeof has type T.Type -> Int
BUILTIN_MISC_OPERATION(Sizeof, "sizeof", "n", Special)

/// Strideof has type T.Type -> Int
BUILTIN_MISC_OPERATION(Strideof, "strideof", "n", Special)

/// IsPOD has type T.Type -> Bool
BUILTIN_MISC_OPERATION(IsPOD, "ispod", "n", Special)

/// IsConcrete has type (T.Type) -> Bool
///
/// If the meta type T is concrete, we can always transform this to `true` at
/// any time in PIL. If it's generic, then we lower it to `false` right before
/// IRGen in IRGenPrepare. This allows for the optimizer to specialize this at
/// -O and eliminate conditional code.
BUILTIN_MISC_OPERATION(IsConcrete, "isConcrete", "n", Special)

/// IsBitwiseTakable has type T.Type -> Bool
BUILTIN_MISC_OPERATION(IsBitwiseTakable, "isbitwisetakable", "n", Special)

/// IsSameMetatype has type (Any.Type, Any.Type) -> Bool
BUILTIN_MISC_OPERATION(IsSameMetatype, "is_same_metatype", "n", Special)

/// Alignof has type T.Type -> Int
BUILTIN_MISC_OPERATION(Alignof, "alignof", "n", Special)

/// AllocRaw has type (Int, Int) -> Builtin.RawPointer
///
/// Parameters: object size, object alignment.
///
/// This alignment is not a mask; the compiler decrements by one to provide
/// a mask to the runtime.
///
/// If alignment == 0, then the runtime will use "aligned" allocation,
/// and the memory will be aligned to _swift_MinAllocationAlignment.
BUILTIN_MISC_OPERATION(AllocRaw, "allocRaw", "", Special)

/// DeallocRaw has type (Builtin.RawPointer, Int, Int) -> ()
///
/// Parameters: object address, object size, object alignment.
///
/// This alignment is not a mask; the compiler decrements by one to provide
/// a mask to the runtime.
///
/// If alignment == 0, then the runtime will use the "aligned" deallocation
/// path, which assumes that "aligned" allocation was used.
///
/// Note that the alignment value provided to `deallocRaw` must be identical to
/// the alignment value provided to `allocRaw` when the memory at this address
/// was allocated.
BUILTIN_MISC_OPERATION(DeallocRaw, "deallocRaw", "", Special)

/// Fence has type () -> ().
BUILTIN_MISC_OPERATION(Fence, "fence", "", None)

/// onFastPath has type () -> ().
BUILTIN_MISC_OPERATION(OnFastPath, "onFastPath", "n", None)

/// CmpXChg has type (Builtin.RawPointer, T, T) -> (T, Bool).
BUILTIN_MISC_OPERATION(CmpXChg, "cmpxchg", "", Special)

/// AtomicLoad has type (Builtin.RawPointer) -> T.
BUILTIN_MISC_OPERATION(AtomicLoad, "atomicload", "", Special)

/// AtomicStore has type (Builtin.RawPointer, T) -> ().
BUILTIN_MISC_OPERATION(AtomicStore, "atomicstore", "", Special)

/// AtomicRMW has type (Builtin.RawPointer, T) -> T.
BUILTIN_MISC_OPERATION(AtomicRMW, "atomicrmw", "", IntegerOrRawPointer)

/// ExtractElement has type (Vector<N, T>, Int32) -> T
BUILTIN_MISC_OPERATION(ExtractElement, "extractelement", "n", Special)

/// InsertElement has type (Vector<N, T>, T, Int32) -> Vector<N, T>.
BUILTIN_MISC_OPERATION(InsertElement, "insertelement", "n", Special)

/// StaticReport has type (Builtin.Int1, Builtin.Int1, Builtin.RawPointer) -> ()
BUILTIN_MISC_OPERATION(StaticReport, "staticReport", "", Special)

/// assert_configuration has type () -> Builtin.Int32
/// Returns the selected assertion configuration.
BUILTIN_MISC_OPERATION(AssertConf, "assert_configuration", "n", Special)

/// StringObjectOr has type (T,T) -> T.
/// Sets bits in a string object. The first operand is bit-cast string literal
/// pointer to an integer. The second operand is the bit mask to be or'd into
/// the high bits of the pointer.
/// It is required that the or'd bits are all 0 in the first operand. So this
/// or-operation is actually equivalent to an addition.
BUILTIN_MISC_OPERATION(StringObjectOr,     "stringObjectOr",      "n", Integer)

/// Special truncation builtins that check for sign and overflow errors. These
/// take an integer as an input and return a tuple of the truncated result and
/// an error bit. The name of each builtin is extended with the "from"
/// (sign-agnostic) builtin integer type and the "to" integer type.
/// We require the source type size to be larger than the destination type size
/// (number of bits).
BUILTIN_MISC_OPERATION(UToSCheckedTrunc, "u_to_s_checked_trunc", "n", Special)
BUILTIN_MISC_OPERATION(SToSCheckedTrunc, "s_to_s_checked_trunc", "n", Special)
BUILTIN_MISC_OPERATION(SToUCheckedTrunc, "s_to_u_checked_trunc", "n", Special)
BUILTIN_MISC_OPERATION(UToUCheckedTrunc, "u_to_u_checked_trunc", "n", Special)

/// IntToFPWithOverflow has type (Integer) -> Float
BUILTIN_MISC_OPERATION(IntToFPWithOverflow, "itofp_with_overflow", "n", Special)

// FIXME: shufflevector

/// zeroInitializer has type <T> () -> T
BUILTIN_MISC_OPERATION(ZeroInitializer, "zeroInitializer", "n", Special)

/// once has type (Builtin.RawPointer, () -> ())
BUILTIN_MISC_OPERATION(Once, "once", "", Special)
/// onceWithContext has type (Builtin.RawPointer, (Builtin.RawPointer) -> (), Builtin.RawPointer)
BUILTIN_MISC_OPERATION(OnceWithContext, "onceWithContext", "", Special)

/// unreachable has type () -> Never
BUILTIN_MISC_OPERATION(Unreachable, "unreachable", "", Special)

/// conditionallyUnreachable has type () -> Never
BUILTIN_MISC_OPERATION(CondUnreachable, "conditionallyUnreachable", "", Special)

/// DestroyArray has type (T.Type, Builtin.RawPointer, Builtin.Word) -> ()
BUILTIN_MISC_OPERATION(DestroyArray, "destroyArray", "", Special)

/// CopyArray, TakeArrayNoAlias, TakeArrayFrontToBack, and TakeArrayBackToFront
/// AssignCopyArrayNoAlias, AssignCopyArrayFrontToBack,
/// AssignCopyArrayBackToFront, AssignTakeArray all have type
/// (T.Type, Builtin.RawPointer, Builtin.RawPointer, Builtin.Word) -> ()
BUILTIN_MISC_OPERATION(CopyArray, "copyArray", "", Special)
BUILTIN_MISC_OPERATION(TakeArrayNoAlias, "takeArrayNoAlias", "", Special)
BUILTIN_MISC_OPERATION(TakeArrayFrontToBack, "takeArrayFrontToBack", "", Special)
BUILTIN_MISC_OPERATION(TakeArrayBackToFront, "takeArrayBackToFront", "", Special)
BUILTIN_MISC_OPERATION(AssignCopyArrayNoAlias, "assignCopyArrayNoAlias", "", Special)
BUILTIN_MISC_OPERATION(AssignCopyArrayFrontToBack, "assignCopyArrayFrontToBack", "", Special)
BUILTIN_MISC_OPERATION(AssignCopyArrayBackToFront, "assignCopyArrayBackToFront", "", Special)
BUILTIN_MISC_OPERATION(AssignTakeArray, "assignTakeArray", "", Special)

// unsafeGuaranteed has type <T: AnyObject> T -> (T, Builtin.Int8)
BUILTIN_MISC_OPERATION(UnsafeGuaranteed, "unsafeGuaranteed", "", Special)

// unsafeGuaranteedEnd has type (Builtin.Int8) -> ()
BUILTIN_MISC_OPERATION(UnsafeGuaranteedEnd, "unsafeGuaranteedEnd", "", Special)

// getObjCTypeEncoding has type <T> T.Type -> RawPointer
BUILTIN_MISC_OPERATION(GetObjCTypeEncoding, "getObjCTypeEncoding", "n", Special)

// Swift3ImplicitObjCEntrypoint has type () -> ()
BUILTIN_MISC_OPERATION(Swift3ImplicitObjCEntrypoint, "swift3ImplicitObjCEntrypoint", "", Special)

/// willThrow: Error -> ()
BUILTIN_MISC_OPERATION(WillThrow, "willThrow", "", Special)

/// poundAssert has type (Builtin.Int1, Builtin.RawPointer) -> ().
BUILTIN_MISC_OPERATION(PoundAssert, "poundAssert", "", Special)

// BUILTIN_MISC_OPERATION_WITH_PILGEN - Miscellaneous operations that are
// specially emitted during PIL generation.
#ifndef BUILTIN_MISC_OPERATION_WITH_PILGEN
#define BUILTIN_MISC_OPERATION_WITH_PILGEN(Id, Name, Attrs, Overload) \
  BUILTIN_MISC_OPERATION(Id, Name, Attrs, Overload)
#endif

/// globalStringTablePointer has type String -> Builtin.RawPointer.
/// It returns an immortal, global string table pointer for strings constructed
/// from string literals.
BUILTIN_MISC_OPERATION_WITH_PILGEN(GlobalStringTablePointer, "globalStringTablePointer", "", Special)

#undef BUILTIN_MISC_OPERATION_WITH_PILGEN

#undef BUILTIN_MISC_OPERATION

/// Builtins for instrumentation added by sanitizers during PILGen.
#ifndef BUILTIN_SANITIZER_OPERATION
#define BUILTIN_SANITIZER_OPERATION(Id, Name, Attrs) BUILTIN(Id, Name, Attrs)
#endif

/// Builtin representing a call to Thread Sanitizer instrumentation.
/// TSanInoutAccess has type (T) -> ()
BUILTIN_SANITIZER_OPERATION(TSanInoutAccess, "tsanInoutAccess", "")

#undef BUILTIN_SANITIZER_OPERATION

/// Builtins for compile-time type-checking operations used for unit testing.
#ifndef BUILTIN_TYPE_CHECKER_OPERATION
#define BUILTIN_TYPE_CHECKER_OPERATION(Id, Name) BUILTIN(Id, #Name, "n")
#endif

BUILTIN_TYPE_CHECKER_OPERATION(TypeJoin, type_join)
BUILTIN_TYPE_CHECKER_OPERATION(TypeJoinInout, type_join_inout)
BUILTIN_TYPE_CHECKER_OPERATION(TypeJoinMeta, type_join_meta)
BUILTIN_TYPE_CHECKER_OPERATION(TriggerFallbackDiagnostic, trigger_fallback_diagnostic)

#undef BUILTIN_TYPE_CHECKER_OPERATION

// BUILTIN_TYPE_TRAIT_OPERATION - Compile-time type trait operations.
#ifndef BUILTIN_TYPE_TRAIT_OPERATION
#define BUILTIN_TYPE_TRAIT_OPERATION(Id, Name) \
  BUILTIN(Id, #Name, "n")
#endif

/// canBeClass(T.Type) -> Builtin.Int8
/// At compile time, evaluate whether T is or can be bound to a class or
/// @objc protocol type. The answer is a tri-state of 0 = No, 1 = Yes, 2 =
/// Maybe.
BUILTIN_TYPE_TRAIT_OPERATION(CanBeObjCClass, canBeClass)

#undef BUILTIN_TYPE_TRAIT_OPERATION

#undef BUILTIN
