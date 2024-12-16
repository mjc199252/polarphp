//===--- KnownDecls.def - Compiler declaration metaprogramming --*- C++ -*-===//
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
// This file defines macros used for macro-metaprogramming with compiler-known
// declarations.
//
//===----------------------------------------------------------------------===//

#ifndef FUNC_DECL
#  define FUNC_DECL(Name, Id)
#endif

FUNC_DECL(ArrayForceCast, "_arrayForceCast")
FUNC_DECL(ArrayConditionalCast, "_arrayConditionalCast")

FUNC_DECL(DictionaryUpCast, "_dictionaryUpCast")
FUNC_DECL(DictionaryDownCast, "_dictionaryDownCast")
FUNC_DECL(DictionaryDownCastConditional, "_dictionaryDownCastConditional")

FUNC_DECL(SetUpCast, "_setUpCast")
FUNC_DECL(SetDownCast, "_setDownCast")
FUNC_DECL(SetDownCastConditional, "_setDownCastConditional")

FUNC_DECL(ConvertPointerToPointerArgument,
          "_convertPointerToPointerArgument")
FUNC_DECL(ConvertInOutToPointerArgument,
          "_convertInOutToPointerArgument")
FUNC_DECL(ConvertMutableArrayToPointerArgument,
          "_convertMutableArrayToPointerArgument")
FUNC_DECL(ConvertConstArrayToPointerArgument,
          "_convertConstArrayToPointerArgument")
FUNC_DECL(ConvertConstStringToUTF8PointerArgument,
          "_convertConstStringToUTF8PointerArgument")
FUNC_DECL(AllocateUninitializedArray,
          "_allocateUninitializedArray")
FUNC_DECL(DeallocateUninitializedArray,
          "_deallocateUninitializedArray")

FUNC_DECL(ForceBridgeFromObjectiveC,
          "_forceBridgeFromObjectiveC")
FUNC_DECL(ConditionallyBridgeFromObjectiveC,
          "_conditionallyBridgeFromObjectiveC")

FUNC_DECL(ForceBridgeFromObjectiveCBridgeable,
          "_forceBridgeFromObjectiveC_bridgeable")
FUNC_DECL(ConditionallyBridgeFromObjectiveCBridgeable,
          "_conditionallyBridgeFromObjectiveC_bridgeable")

FUNC_DECL(BridgeAnythingToObjectiveC,
          "_bridgeAnythingToObjectiveC")
FUNC_DECL(BridgeAnyObjectToAny,
          "_bridgeAnyObjectToAny")

FUNC_DECL(ConvertToAnyHashable, "_convertToAnyHashable")

FUNC_DECL(DiagnoseUnexpectedError, "_unexpectedError")
FUNC_DECL(DiagnoseUnexpectedNilOptional, "_diagnoseUnexpectedNilOptional")
FUNC_DECL(DiagnoseUnexpectedEnumCase, "_diagnoseUnexpectedEnumCase")
FUNC_DECL(DiagnoseUnexpectedEnumCaseValue, "_diagnoseUnexpectedEnumCaseValue")

FUNC_DECL(GetErrorEmbeddedNSError, "_getErrorEmbeddedNSError")

FUNC_DECL(UnsafeBitCast, "unsafeBitCast")

FUNC_DECL(GetAtKeyPath, "_getAtKeyPath")
FUNC_DECL(GetAtAnyKeyPath, "_getAtAnyKeyPath")
FUNC_DECL(GetAtPartialKeyPath, "_getAtPartialKeyPath")
FUNC_DECL(SetAtWritableKeyPath, "_setAtWritableKeyPath")
FUNC_DECL(SetAtReferenceWritableKeyPath, "_setAtReferenceWritableKeyPath")
// These don't actually have AST nodes associated with them right now.
FUNC_DECL(ReadAtKeyPath, "_readAtKeyPath")
FUNC_DECL(ModifyAtWritableKeyPath, "_modifyAtWritableKeyPath")
FUNC_DECL(ModifyAtReferenceWritableKeyPath, "_modifyAtReferenceWritableKeyPath")

FUNC_DECL(Swap, "swap")

FUNC_DECL(UnimplementedInitializer, "_unimplementedInitializer")
FUNC_DECL(Undefined, "_undefined")

#undef FUNC_DECL
