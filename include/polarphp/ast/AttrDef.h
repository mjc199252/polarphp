//===--- Attr.def - Swift Attributes Metaprogramming ------------*- C++ -*-===//
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
// This file defines macros used for macro-metaprogramming with attributes.
//
//===----------------------------------------------------------------------===//

#ifndef DECL_ATTR
#define DECL_ATTR(SPELLING, CLASS, OPTIONS, CODE)
#endif

#ifndef CONTEXTUAL_DECL_ATTR
#define CONTEXTUAL_DECL_ATTR(SPELLING, CLASS, OPTIONS, CODE) \
                   DECL_ATTR(SPELLING, CLASS, OPTIONS, CODE)
#endif

#ifndef SIMPLE_DECL_ATTR
#define SIMPLE_DECL_ATTR(X, CLASS, OPTIONS, CODE) \
               DECL_ATTR(X, CLASS, OPTIONS, CODE)
#endif

#ifndef CONTEXTUAL_SIMPLE_DECL_ATTR
#define CONTEXTUAL_SIMPLE_DECL_ATTR(X, CLASS, OPTIONS, CODE) \
                   SIMPLE_DECL_ATTR(X, CLASS, OPTIONS, CODE)
#endif

#ifndef DECL_ATTR_ALIAS
#define DECL_ATTR_ALIAS(SPELLING, CLASS)
#endif

#ifndef CONTEXTUAL_DECL_ATTR_ALIAS
#define CONTEXTUAL_DECL_ATTR_ALIAS(SPELLING, CLASS) \
                   DECL_ATTR_ALIAS(SPELLING, CLASS)
#endif

#ifndef TYPE_ATTR
#define TYPE_ATTR(X)
#endif

// Type attributes
TYPE_ATTR(autoclosure)
TYPE_ATTR(convention)
TYPE_ATTR(noescape)
TYPE_ATTR(escaping)
TYPE_ATTR(differentiable)

// PIL-specific attributes
TYPE_ATTR(block_storage)
TYPE_ATTR(box)
TYPE_ATTR(dynamic_self)
#define REF_STORAGE(Name, name, ...) TYPE_ATTR(pil_##name)
#include "polarphp/ast/ReferenceStorageDef.h"
TYPE_ATTR(error)
TYPE_ATTR(out)
TYPE_ATTR(in)
TYPE_ATTR(inout)
TYPE_ATTR(inout_aliasable)
TYPE_ATTR(in_guaranteed)
TYPE_ATTR(in_constant)
TYPE_ATTR(owned)
TYPE_ATTR(unowned_inner_pointer)
TYPE_ATTR(guaranteed)
TYPE_ATTR(autoreleased)
TYPE_ATTR(callee_owned)
TYPE_ATTR(callee_guaranteed)
TYPE_ATTR(opened)
TYPE_ATTR(pseudogeneric)
TYPE_ATTR(yields)
TYPE_ATTR(yield_once)
TYPE_ATTR(yield_many)

// PIL metatype attributes.
TYPE_ATTR(thin)
TYPE_ATTR(thick)

// Generated interface attributes
TYPE_ATTR(_opaqueReturnTypeOf)

// Schema for DECL_ATTR:
//
// - Attribute name.
// - Class name without the 'Attr' suffix (ignored for
// - Options for the attribute, including:
//    * the declarations the attribute can appear on
//    * whether duplicates are allowed
//    * whether the attribute is considered a decl modifier or not (no '@')
// - Unique attribute identifier used for serialization.  This
//   can never be changed.
//
// SIMPLE_DECL_ATTR is the same, but the class becomes
// SimpleDeclAttr<DAK_##NAME>.
//
// Please help ease code review/audits:
// - Please indent once, not to the opening '('.
// - Please place the "OnXYZ" flags together on the next line.
// - Please place the non-OnXYZ flags together on the next to last line.
// - Please place the unique number on the last line. If the attribute is NOT
//   serialized, then please place that flag on the last line too. For example:
//     123)
//     NotSerialized, 321)
// - Please sort attributes by serialization number.
// - Please create a "NOTE" comment if a unique number is skipped.

DECL_ATTR(_pilgen_name, PILGenName,
          OnAbstractFunction |
          LongAttribute | UserInaccessible | ABIStableToAdd | ABIStableToRemove |
          APIStableToAdd | APIStableToRemove,
          0)
DECL_ATTR(available, Available,
          OnAbstractFunction | OnGenericType | OnVar | OnSubscript | OnEnumElement |
          OnExtension | OnGenericTypeParam |
          AllowMultipleAttributes | LongAttribute |
          ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
          1)
CONTEXTUAL_SIMPLE_DECL_ATTR(final, Final,
                            OnClass | OnFunc | OnAccessor | OnVar | OnSubscript |
                            DeclModifier |
                            ABIBreakingToAdd | ABIBreakingToRemove | APIBreakingToAdd | APIStableToRemove,
                            2)
CONTEXTUAL_SIMPLE_DECL_ATTR(required, Required,
                            OnConstructor |
                            DeclModifier |
                            ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                            4)
CONTEXTUAL_SIMPLE_DECL_ATTR(optional, Optional,
                            OnConstructor | OnFunc | OnAccessor | OnVar | OnSubscript |
                            DeclModifier |
                            ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                            5)
SIMPLE_DECL_ATTR(dynamicCallable, DynamicCallable,
                 OnNominalType |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 6)
// NOTE: 7 is unused
SIMPLE_DECL_ATTR(_exported, Exported,
                 OnImport |
                 UserInaccessible |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 8)
SIMPLE_DECL_ATTR(dynamicMemberLookup, DynamicMemberLookup,
                 OnNominalType |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 9)
SIMPLE_DECL_ATTR(NSCopying, NSCopying,
                 OnVar |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 10)
SIMPLE_DECL_ATTR(IBAction, IBAction,
                 OnFunc |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 11)
SIMPLE_DECL_ATTR(IBDesignable, IBDesignable,
                 OnClass | OnExtension |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 12)
SIMPLE_DECL_ATTR(IBInspectable, IBInspectable,
                 OnVar |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 13)
SIMPLE_DECL_ATTR(IBOutlet, IBOutlet,
                 OnVar |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 14)
SIMPLE_DECL_ATTR(NSManaged, NSManaged,
                 OnVar | OnFunc | OnAccessor |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 15)
CONTEXTUAL_SIMPLE_DECL_ATTR(lazy, Lazy, DeclModifier |
                                        OnVar |
                                        ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                            16)
SIMPLE_DECL_ATTR(LLDBDebuggerFunction, LLDBDebuggerFunction,
                 OnFunc |
                 UserInaccessible |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 17)
SIMPLE_DECL_ATTR(UIApplicationMain, UIApplicationMain,
                 OnClass |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 18)
//SIMPLE_DECL_ATTR(unsafe_no_objc_tagged_pointer, UnsafeNoObjCTaggedPointer,
//                 OnInterface |
//                 UserInaccessible |
//                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
//                 19)
DECL_ATTR(inline, Inline,
          OnVar | OnSubscript | OnAbstractFunction |
          ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
          20)
DECL_ATTR(_semantics, Semantics,
          OnAbstractFunction | OnSubscript | OnNominalType | OnVar |
          AllowMultipleAttributes | UserInaccessible |
          ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
          21)
CONTEXTUAL_SIMPLE_DECL_ATTR(dynamic, Dynamic,
                            OnFunc | OnAccessor | OnVar | OnSubscript | OnConstructor |
                            DeclModifier | ABIBreakingToAdd | ABIBreakingToRemove |
                            APIStableToAdd | APIStableToRemove,
                            22)
CONTEXTUAL_SIMPLE_DECL_ATTR(infix, Infix,
                            OnFunc | OnOperator |
                            DeclModifier |
                            ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                            23)
CONTEXTUAL_SIMPLE_DECL_ATTR(prefix, Prefix,
                            OnFunc | OnOperator |
                            DeclModifier |
                            ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                            24)
CONTEXTUAL_SIMPLE_DECL_ATTR(postfix, Postfix,
                            OnFunc | OnOperator |
                            DeclModifier |
                            ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                            25)
SIMPLE_DECL_ATTR(_transparent, Transparent,
                 OnFunc | OnAccessor | OnConstructor | OnVar | UserInaccessible |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 26)
SIMPLE_DECL_ATTR(requires_stored_property_inits, RequiresStoredPropertyInits,
                 OnClass |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 27)
//SIMPLE_DECL_ATTR(nonobjc, NonObjC,
//                 OnExtension | OnFunc | OnAccessor | OnVar | OnSubscript | OnConstructor |
//                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
//                 30)
SIMPLE_DECL_ATTR(_fixed_layout, FixedLayout,
                 OnVar | OnClass | OnStruct |
                 UserInaccessible | ABIBreakingToAdd | ABIBreakingToRemove |
                 APIStableToAdd | APIStableToRemove,
                 31)
SIMPLE_DECL_ATTR(inlinable, Inlinable,
                 OnVar | OnSubscript | OnAbstractFunction |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 32)
DECL_ATTR(_specialize, Specialize,
          OnConstructor | OnFunc | OnAccessor |
          AllowMultipleAttributes | LongAttribute | UserInaccessible |
          ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
          33)
//SIMPLE_DECL_ATTR(objcMembers, ObjCMembers,
//                 OnClass |
//                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
//                 34)
CONTEXTUAL_SIMPLE_DECL_ATTR(__consuming, Consuming,
                            OnFunc | OnAccessor |
                            DeclModifier |
                            UserInaccessible |
                            NotSerialized |
                            ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove, 40)
CONTEXTUAL_SIMPLE_DECL_ATTR(mutating, Mutating,
                            OnFunc | OnAccessor |
                            DeclModifier |
                            NotSerialized |
                            ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove, 41)
CONTEXTUAL_SIMPLE_DECL_ATTR(nonmutating, NonMutating,
                            OnFunc | OnAccessor |
                            DeclModifier |
                            NotSerialized |
                            ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove, 42)
CONTEXTUAL_SIMPLE_DECL_ATTR(convenience, Convenience,
                            OnConstructor |
                            DeclModifier |
                            NotSerialized |
                            ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove, 43)
CONTEXTUAL_SIMPLE_DECL_ATTR(override, Override,
                            OnFunc | OnAccessor | OnVar | OnSubscript | OnConstructor | OnAssociatedType |
                            DeclModifier |
                            NotSerialized |
                            ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove, 44)
SIMPLE_DECL_ATTR(_hasStorage, HasStorage,
                 OnVar |
                 UserInaccessible |
                 NotSerialized |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove, 45)
DECL_ATTR(private, AccessControl,
          OnFunc | OnAccessor | OnExtension | OnGenericType | OnVar | OnSubscript |
          OnConstructor |
          DeclModifier |
          NotSerialized |
          ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove, 46)
DECL_ATTR_ALIAS(fileprivate, AccessControl)
DECL_ATTR_ALIAS(internal, AccessControl)
DECL_ATTR_ALIAS(public, AccessControl)
CONTEXTUAL_DECL_ATTR_ALIAS(open, AccessControl)
DECL_ATTR(__setter_access, SetterAccess,
          OnVar | OnSubscript |
          DeclModifier | RejectByParser |
          NotSerialized |
          ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove, 47)
DECL_ATTR(__raw_doc_comment, RawDocComment,
          OnAnyDecl |
          RejectByParser |
          NotSerialized |
          ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove, 48)
CONTEXTUAL_DECL_ATTR(weak, ReferenceOwnership,
                     OnVar |
                     DeclModifier |
                     NotSerialized |
                     ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove, 49)
CONTEXTUAL_DECL_ATTR_ALIAS(unowned, ReferenceOwnership)
DECL_ATTR(_effects, Effects,
          OnAbstractFunction |
          UserInaccessible |
          ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
          50)
//DECL_ATTR(__objc_bridged, ObjCBridged,
//          OnClass |
//          RejectByParser |
//          NotSerialized |
//          ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
//          51)
SIMPLE_DECL_ATTR(NSApplicationMain, NSApplicationMain,
                 OnClass |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 52)
//SIMPLE_DECL_ATTR(_objc_non_lazy_realization, ObjCNonLazyRealization,
//                 OnClass |
//                 UserInaccessible |
//                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
//                 53)
DECL_ATTR(__synthesized_protocol, SynthesizedInterface,
          OnConcreteNominalType |
          RejectByParser |
          NotSerialized |
          ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove, 54)
SIMPLE_DECL_ATTR(testable, Testable,
                 OnImport |
                 UserInaccessible |
                 NotSerialized |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 55)
DECL_ATTR(_alignment, Alignment,
          OnStruct | OnEnum |
          UserInaccessible |
          ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
          56)
SIMPLE_DECL_ATTR(rethrows, Rethrows,
                 OnFunc | OnAccessor | OnConstructor |
                 RejectByParser |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 57)
//DECL_ATTR(_swift_native_objc_runtime_base, SwiftNativeObjCRuntimeBase,
//          OnClass |
//          UserInaccessible |
//          ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
//          59)
CONTEXTUAL_SIMPLE_DECL_ATTR(indirect, Indirect, DeclModifier |
                                                OnEnum | OnEnumElement |
                                                ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                            60)
SIMPLE_DECL_ATTR(warn_unqualified_access, WarnUnqualifiedAccess,
                 OnFunc | OnAccessor /*| OnVar*/ |
                 LongAttribute |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 61)
SIMPLE_DECL_ATTR(_show_in_interface, ShowInInterface,
                 OnInterface |
                 UserInaccessible |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 62)
DECL_ATTR(_cdecl, CDecl,
          OnFunc | OnAccessor |
          LongAttribute | UserInaccessible |
          ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
          63)
SIMPLE_DECL_ATTR(usableFromInline, UsableFromInline,
                 OnAbstractFunction | OnVar | OnSubscript | OnNominalType | OnTypeAlias |
                 LongAttribute |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 64)
SIMPLE_DECL_ATTR(discardableResult, DiscardableResult,
                 OnFunc | OnAccessor | OnConstructor |
                 LongAttribute |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 65)
SIMPLE_DECL_ATTR(GKInspectable, GKInspectable,
                 OnVar |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 66)
DECL_ATTR(_implements, Implements,
          OnFunc | OnAccessor | OnVar | OnSubscript | OnTypeAlias |
          UserInaccessible |
          NotSerialized |
          ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
          67)

// NOTE: 71 is unused
// NOTE: 72 is unused
DECL_ATTR(_optimize, Optimize,
          OnAbstractFunction | OnSubscript | OnVar |
          UserInaccessible |
          ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
          73)
DECL_ATTR(_clangImporterSynthesizedType, ClangImporterSynthesizedType,
          OnGenericType |
          LongAttribute | RejectByParser | UserInaccessible |
          NotSerialized |
          ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
          74)
SIMPLE_DECL_ATTR(_weakLinked, WeakLinked,
                 OnNominalType | OnAssociatedType | OnFunc | OnAccessor | OnVar |
                 OnSubscript | OnConstructor | OnEnumElement | OnExtension | UserInaccessible |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 75)
SIMPLE_DECL_ATTR(frozen, Frozen,
                 OnEnum | OnStruct | ABIBreakingToAdd | ABIBreakingToRemove | APIBreakingToRemove | APIStableToAdd,
                 76)
DECL_ATTR_ALIAS(_frozen, Frozen)
SIMPLE_DECL_ATTR(_forbidSerializingReference, ForbidSerializingReference,
                 OnAnyDecl |
                 LongAttribute | RejectByParser | UserInaccessible | NotSerialized |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 77)
SIMPLE_DECL_ATTR(_hasInitialValue, HasInitialValue,
                 OnVar |
                 UserInaccessible |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 78)
SIMPLE_DECL_ATTR(_nonoverride, NonOverride,
                 OnFunc | OnAccessor | OnVar | OnSubscript | OnConstructor | OnAssociatedType |
                 UserInaccessible | NotSerialized |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 79)
DECL_ATTR(_dynamicReplacement, DynamicReplacement,
          OnAbstractFunction | OnVar | OnSubscript | UserInaccessible |
          ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
          80)
SIMPLE_DECL_ATTR(_borrowed, Borrowed,
                 OnVar | OnSubscript | UserInaccessible |
                 NotSerialized |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 81)
DECL_ATTR(_private, PrivateImport,
          OnImport |
          UserInaccessible |
          NotSerialized |
          ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
          82)
SIMPLE_DECL_ATTR(_alwaysEmitIntoClient, AlwaysEmitIntoClient,
                 OnVar | OnSubscript | OnAbstractFunction | UserInaccessible |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 83)

SIMPLE_DECL_ATTR(_implementationOnly, ImplementationOnly,
                 OnImport | OnFunc | OnConstructor | OnVar | OnSubscript | UserInaccessible |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 84)
DECL_ATTR(_custom, Custom,
          OnAnyDecl | UserInaccessible |
          ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
          85)
SIMPLE_DECL_ATTR(propertyWrapper, PropertyWrapper,
                 OnStruct | OnClass | OnEnum |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 86)
SIMPLE_DECL_ATTR(_disfavoredOverload, DisfavoredOverload,
                 OnAbstractFunction | OnVar | OnSubscript | UserInaccessible |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 87)
SIMPLE_DECL_ATTR(_functionBuilder, FunctionBuilder,
                 OnNominalType |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 88)
DECL_ATTR(_projectedValueProperty, ProjectedValueProperty,
          OnVar | UserInaccessible |
          ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
          89)
SIMPLE_DECL_ATTR(_nonEphemeral, NonEphemeral,
                 OnParam | UserInaccessible |
                 ABIStableToAdd | ABIStableToRemove | APIBreakingToAdd | APIStableToRemove,
                 90)

DECL_ATTR(differentiable, Differentiable,
          OnAccessor | OnConstructor | OnFunc | OnVar | OnSubscript | LongAttribute |
          AllowMultipleAttributes |
          ABIStableToAdd | ABIBreakingToRemove | APIStableToAdd | APIBreakingToRemove,
          91)

SIMPLE_DECL_ATTR(_hasMissingDesignatedInitializers,
                 HasMissingDesignatedInitializers, OnClass | UserInaccessible | NotSerialized |
                                                   APIBreakingToAdd | ABIBreakingToAdd | APIStableToRemove | ABIStableToRemove,
                 92)

SIMPLE_DECL_ATTR(_inheritsConvenienceInitializers,
                 InheritsConvenienceInitializers, OnClass | UserInaccessible | NotSerialized |
                                                  APIStableToAdd | ABIStableToAdd | APIBreakingToRemove | ABIBreakingToRemove,
                 93)

SIMPLE_DECL_ATTR(IBSegueAction, IBSegueAction,
                 OnFunc |
                 ABIStableToAdd | ABIStableToRemove | APIStableToAdd | APIStableToRemove,
                 95)

DECL_ATTR(_originallyDefinedIn, OriginallyDefinedIn,
          OnNominalType | OnFunc | OnVar | OnExtension | UserInaccessible |
          AllowMultipleAttributes |
          ABIBreakingToAdd | ABIBreakingToRemove | APIStableToAdd | APIStableToRemove,
          96)

#undef TYPE_ATTR
#undef DECL_ATTR_ALIAS
#undef CONTEXTUAL_DECL_ATTR_ALIAS
#undef SIMPLE_DECL_ATTR
#undef CONTEXTUAL_SIMPLE_DECL_ATTR
#undef DECL_ATTR
#undef CONTEXTUAL_DECL_ATTR
