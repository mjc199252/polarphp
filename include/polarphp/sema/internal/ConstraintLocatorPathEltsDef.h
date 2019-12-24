//===--- ConstraintLocatorPathElts.def - Constraint Locator Path Elements -===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
///
/// This file enumerates the elements that can make up the path of a
/// ConstraintLocator.
///
//===----------------------------------------------------------------------===//

/// Describes any kind of path element.
#ifndef LOCATOR_PATH_ELT
#define LOCATOR_PATH_ELT(Name)
#endif

/// Defines a path element which is characterized only by its kind, and as such
/// doesn't store additional values.
#ifndef SIMPLE_LOCATOR_PATH_ELT
#define SIMPLE_LOCATOR_PATH_ELT(Name) LOCATOR_PATH_ELT(Name)
#endif

/// Defines a path element that requires a class definition to be provided in
/// order to expose things like accessors for path element info.
#ifndef CUSTOM_LOCATOR_PATH_ELT
#define CUSTOM_LOCATOR_PATH_ELT(Name) LOCATOR_PATH_ELT(Name)
#endif

/// Defines an abstract path element superclass which doesn't itself have a path
/// element kind.
#ifndef ABSTRACT_LOCATOR_PATH_ELT
#define ABSTRACT_LOCATOR_PATH_ELT(Name) CUSTOM_LOCATOR_PATH_ELT(Name)
#endif

/// Matching an argument to a parameter.
CUSTOM_LOCATOR_PATH_ELT(ApplyArgToParam)

/// The argument of function application.
SIMPLE_LOCATOR_PATH_ELT(ApplyArgument)

/// The function being applied.
SIMPLE_LOCATOR_PATH_ELT(ApplyFunction)

/// An argument passed in an autoclosure parameter
/// position, which must match the autoclosure return type.
SIMPLE_LOCATOR_PATH_ELT(AutoclosureResult)

/// The result of a closure.
SIMPLE_LOCATOR_PATH_ELT(ClosureResult)

/// The lookup for a constructor member.
SIMPLE_LOCATOR_PATH_ELT(ConstructorMember)

/// The desired contextual type passed in to the constraint system.
CUSTOM_LOCATOR_PATH_ELT(ContextualType)

/// A result of an expression involving dynamic lookup.
SIMPLE_LOCATOR_PATH_ELT(DynamicLookupResult)

/// The superclass of a protocol existential type.
SIMPLE_LOCATOR_PATH_ELT(ExistentialSuperclassType)

/// The argument type of a function.
SIMPLE_LOCATOR_PATH_ELT(FunctionArgument)

/// The result type of a function.
SIMPLE_LOCATOR_PATH_ELT(FunctionResult)

/// A generic argument.
/// FIXME: Add support for named generic arguments?
CUSTOM_LOCATOR_PATH_ELT(GenericArgument)

/// Locator for a binding from an IUO disjunction choice.
SIMPLE_LOCATOR_PATH_ELT(ImplicitlyUnwrappedDisjunctionChoice)

/// The instance of a metatype type.
SIMPLE_LOCATOR_PATH_ELT(InstanceType)

/// A generic parameter being opened.
///
/// Also contains the generic parameter type itself.
CUSTOM_LOCATOR_PATH_ELT(GenericParameter)

/// A component of a key path.
CUSTOM_LOCATOR_PATH_ELT(KeyPathComponent)

/// The result type of a key path component. Not used for subscripts.
SIMPLE_LOCATOR_PATH_ELT(KeyPathComponentResult)

/// The member looked up via keypath based dynamic lookup.
CUSTOM_LOCATOR_PATH_ELT(KeyPathDynamicMember)

/// The root of a key path.
SIMPLE_LOCATOR_PATH_ELT(KeyPathRoot)

/// The type of the key path expression.
SIMPLE_LOCATOR_PATH_ELT(KeyPathType)

/// The value of a key path.
SIMPLE_LOCATOR_PATH_ELT(KeyPathValue)

/// An implicit @lvalue-to-inout conversion; only valid for operator
/// arguments.
SIMPLE_LOCATOR_PATH_ELT(LValueConversion)

/// A member.
/// FIXME: Do we need the actual member name here?
SIMPLE_LOCATOR_PATH_ELT(Member)

/// The base of a member expression.
SIMPLE_LOCATOR_PATH_ELT(MemberRefBase)

/// This is referring to a type produced by opening a generic type at the
/// base of the locator.
CUSTOM_LOCATOR_PATH_ELT(OpenedGeneric)

/// An optional payload.
SIMPLE_LOCATOR_PATH_ELT(OptionalPayload)

/// The parent of a nested type.
SIMPLE_LOCATOR_PATH_ELT(ParentType)

/// The requirement that we're matching during protocol conformance
/// checking.
CUSTOM_LOCATOR_PATH_ELT(InterfaceRequirement)

/// Type parameter requirements.
ABSTRACT_LOCATOR_PATH_ELT(AnyRequirement)
/// The Nth conditional requirement in the parent locator's conformance.
CUSTOM_LOCATOR_PATH_ELT(ConditionalRequirement)

/// A single requirement placed on the type parameters.
CUSTOM_LOCATOR_PATH_ELT(TypeParameterRequirement)

/// RValue adjustment.
SIMPLE_LOCATOR_PATH_ELT(RValueAdjustment)

/// The element type of a sequence in a for ... in ... loop.
SIMPLE_LOCATOR_PATH_ELT(SequenceElementType)

/// The lookup for a subscript member.
SIMPLE_LOCATOR_PATH_ELT(SubscriptMember)

/// The missing argument synthesized by the solver.
CUSTOM_LOCATOR_PATH_ELT(SynthesizedArgument)

/// Tuple elements.
ABSTRACT_LOCATOR_PATH_ELT(AnyTupleElement)
/// A tuple element referenced by position.
CUSTOM_LOCATOR_PATH_ELT(TupleElement)

/// A tuple element referenced by name.
CUSTOM_LOCATOR_PATH_ELT(NamedTupleElement)

/// An unresolved member.
SIMPLE_LOCATOR_PATH_ELT(UnresolvedMember)

/// The candidate witness during protocol conformance checking.
CUSTOM_LOCATOR_PATH_ELT(Witness)

/// The condition associated with 'if' expression or ternary operator.
SIMPLE_LOCATOR_PATH_ELT(Condition)

#undef LOCATOR_PATH_ELT
#undef CUSTOM_LOCATOR_PATH_ELT
#undef SIMPLE_LOCATOR_PATH_ELT
#undef ABSTRACT_LOCATOR_PATH_ELT
