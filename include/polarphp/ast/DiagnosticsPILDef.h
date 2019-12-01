//===--- DiagnosticsSIL.def - Diagnostics Text ------------------*- C++ -*-===//
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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/12/01.
//===----------------------------------------------------------------------===//
//  This file defines diagnostics emitted during SIL (dataflow) analysis.
//  Each diagnostic is described using one of three kinds (error, warning, or
//  note) along with a unique identifier, category, options, and text, and is
//  followed by a signature describing the diagnostic argument kinds.
//
//===----------------------------------------------------------------------===//

#if !(defined(DIAG) || (defined(ERROR) && defined(WARNING) && defined(NOTE) && defined(REMARK)))
#  error Must define either DIAG or the set {ERROR,WARNING,NOTE,REMARK}
#endif

#ifndef ERROR
#  define ERROR(ID,Options,Text,Signature)   \
  DIAG(ERROR,ID,Options,Text,Signature)
#endif

#ifndef WARNING
#  define WARNING(ID,Options,Text,Signature) \
  DIAG(WARNING,ID,Options,Text,Signature)
#endif

#ifndef NOTE
#  define NOTE(ID,Options,Text,Signature) \
  DIAG(NOTE,ID,Options,Text,Signature)
#endif

#ifndef REMARK
#  define REMARK(ID,Options,Text,Signature) \
  DIAG(REMARK,ID,Options,Text,Signature)
#endif


// SILGen issues.
ERROR(bridging_module_missing,NoneType,
      "unable to find module '%0' for implicit conversion function '%0.%1'",
      (StringRef, StringRef))
ERROR(bridging_function_missing,NoneType,
      "unable to find implicit conversion function '%0.%1'",
      (StringRef, StringRef))
ERROR(bridging_function_overloaded,NoneType,
      "multiple definitions of implicit conversion function '%0.%1'",
      (StringRef, StringRef))
ERROR(bridging_function_not_function,NoneType,
      "definition of implicit conversion function '%0.%1' is not a function",
      (StringRef, StringRef))
ERROR(bridging_function_not_correct_type,NoneType,
      "definition of implicit conversion function '%0.%1' is not of the correct"
      " type",
      (StringRef, StringRef))
ERROR(bridging_objcbridgeable_missing,NoneType,
      "cannot find definition of '_ObjectiveCBridgeable' protocol", ())
ERROR(bridging_objcbridgeable_broken,NoneType,
      "broken definition of '_ObjectiveCBridgeable' protocol: missing %0",
      (DeclName))

ERROR(invalid_sil_builtin,NoneType,
      "INTERNAL ERROR: invalid use of builtin: %0",
      (StringRef))
ERROR(could_not_find_bridge_type,NoneType,
      "could not find Objective-C bridge type for type %0; "
      "did you forget to import Foundation?", (Type))
ERROR(could_not_find_pointer_pointee_property,NoneType,
      "could not find 'pointee' property of pointer type %0", (Type))

ERROR(writeback_overlap_property,NoneType,
      "inout writeback to computed property %0 occurs in multiple arguments to"
      " call, introducing invalid aliasing", (Identifier))
ERROR(writeback_overlap_subscript,NoneType,
      "inout writeback through subscript occurs in multiple arguments to call,"
      " introducing invalid aliasing",
      ())
NOTE(writebackoverlap_note,NoneType,
      "concurrent writeback occurred here", ())

ERROR(inout_argument_alias,NoneType,
      "inout arguments are not allowed to alias each other", ())
NOTE(previous_inout_alias,NoneType,
      "previous aliasing argument", ())

ERROR(unimplemented_generator_witnesses,NoneType,
      "protocol conformance emission for generator coroutines is unimplemented",
      ())

ERROR(exclusivity_access_required,NoneType,
      "overlapping accesses to %0, but "
      "%select{initialization|read|modification|deinitialization}1 requires "
      "exclusive access; "
      "%select{consider copying to a local variable|"
              "consider calling MutableCollection.swapAt(_:_:)}2",
      (StringRef, unsigned, bool))

ERROR(exclusivity_access_required_unknown_decl,NoneType,
        "overlapping accesses, but "
        "%select{initialization|read|modification|deinitialization}0 requires "
        "exclusive access; consider copying to a local variable", (unsigned))

NOTE(exclusivity_conflicting_access,NoneType,
     "conflicting access is here", ())

ERROR(unsupported_c_function_pointer_conversion,NoneType,
      "C function pointer signature %0 is not compatible with expected type %1",
      (Type, Type))

ERROR(objc_selector_malformed,NoneType,"the type ObjectiveC.Selector is malformed",
      ())

// Invalid escaping capture diagnostics.
ERROR(escaping_inout_capture,NoneType,
      "escaping closure captures 'inout' parameter %0", (Identifier))
NOTE(inout_param_defined_here,NoneType,
     "parameter %0 is declared 'inout'", (Identifier))
ERROR(escaping_mutable_self_capture,NoneType,
      "escaping closure captures mutating 'self' parameter", ())

ERROR(escaping_noescape_param_capture,NoneType,
      "escaping closure captures non-escaping parameter %0", (Identifier))
NOTE(noescape_param_defined_here,NoneType,
     "parameter %0 is implicitly non-escaping", (Identifier))

ERROR(escaping_noescape_var_capture,NoneType,
      "escaping closure captures non-escaping value", ())

NOTE(value_captured_here,NoneType,"captured here", ())

NOTE(value_captured_transitively,NoneType,
     "captured indirectly by this call", ())

ERROR(err_noescape_param_call,NoneType,
      "passing a %select{|closure which captures a }1non-escaping function "
      "parameter %0 to a call to a non-escaping function parameter can allow "
      "re-entrant modification of a variable",
      (DeclName, unsigned))

// Definite initialization diagnostics.
NOTE(variable_defined_here,NoneType,
     "%select{variable|constant}0 defined here", (bool))
ERROR(variable_used_before_initialized,NoneType,
      "%select{variable|constant}1 '%0' used before being initialized",
      (StringRef, bool))
ERROR(variable_inout_before_initialized,NoneType,
      "%select{variable|constant}1 '%0' passed by reference before being"
      " initialized", (StringRef, bool))
ERROR(variable_closure_use_uninit,NoneType,
      "%select{variable|constant}1 '%0' captured by a closure before being"
      " initialized", (StringRef, bool))
ERROR(variable_defer_use_uninit,NoneType,
      "%select{variable|constant}1 '%0' used in defer before being"
      " initialized", (StringRef, bool))
ERROR(self_closure_use_uninit,NoneType,
      "'self' captured by a closure before all members were initialized", ())


ERROR(variable_addrtaken_before_initialized,NoneType,
      "address of %select{variable|constant}1 '%0' taken before it is"
      " initialized", (StringRef, bool))
ERROR(ivar_not_initialized_at_superinit,NoneType,
      "property '%0' not initialized at super.init call", (StringRef, bool))
ERROR(ivar_not_initialized_at_implicit_superinit,NoneType,
      "property '%0' not initialized at implicitly generated super.init call",
      (StringRef, bool))

ERROR(self_use_before_fully_init,NoneType,
      "'self' used in %select{method call|property access}1 %0 before "
      "%select{all stored properties are initialized|"
      "'super.init' call|"
      "'self.init' call}2", (DeclBaseName, bool, unsigned))
ERROR(use_of_self_before_fully_init,NoneType,
      "'self' used before all stored properties are initialized", ())


NOTE(stored_property_not_initialized,NoneType,
     "'%0' not initialized", (StringRef))

ERROR(selfinit_multiple_times,NoneType,
      "'%select{super|self}0.init' called multiple times in initializer",
      (unsigned))
ERROR(superselfinit_not_called_before_return,NoneType,
      "'%select{super|self}0.init' isn't called on all paths before returning "
      "from initializer", (unsigned))
ERROR(self_before_superinit,NoneType,
      "'self' used before 'super.init' call", ())
ERROR(self_before_selfinit,NoneType,
      "'self' used before 'self.init' call", ())
ERROR(self_before_selfinit_value_type,NoneType,
      "'self' used before 'self.init' call or assignment to 'self'", ())
ERROR(self_inside_catch_superselfinit,NoneType,
      "'self' used inside 'catch' block reachable from "
      "%select{super|self}0.init call",
      (unsigned))
ERROR(return_from_init_without_initing_self,NoneType,
      "return from initializer before 'self.init' call or assignment to 'self'", ())
ERROR(return_from_init_without_initing_stored_properties,NoneType,
      "return from initializer without initializing all"
      " stored properties", ())

ERROR(variable_function_use_uninit,NoneType,
      "%select{variable|constant}1 '%0' used by function definition before"
      " being initialized",
      (StringRef, bool))
ERROR(struct_not_fully_initialized,NoneType,
      "struct '%0' must be completely initialized before a member is stored to",
      (StringRef, bool))
ERROR(immutable_property_already_initialized,NoneType,
      "immutable value '%0' may only be initialized once",
      (StringRef))
NOTE(initial_value_provided_in_let_decl,NoneType,
     "initial value already provided in 'let' declaration", ())
ERROR(mutation_of_property_of_immutable_value,NoneType,
      "cannot mutate %select{property %0|subscript}1 of immutable value '%2'",
      (DeclBaseName, bool, StringRef))
ERROR(using_mutating_accessor_on_immutable_value,NoneType,
      "mutating accessor for %select{property %0|subscript}1 may not"
      " be used on immutable value '%2'",
      (DeclBaseName, bool, StringRef))
ERROR(mutating_method_called_on_immutable_value,NoneType,
      "mutating %select{method|operator}1 %0 may not"
      " be used on immutable value '%2'",
      (DeclBaseName, unsigned, StringRef))
ERROR(immutable_value_passed_inout,NoneType,
      "immutable value '%0' must not be passed inout",
      (StringRef))
ERROR(assignment_to_immutable_value,NoneType,
      "immutable value '%0' must not be assigned to",
      (StringRef))

WARNING(designated_init_in_cross_module_extension,NoneType,
        "initializer for struct %0 must use \"self.init(...)\" or \"self = ...\""
        "%select{| on all paths}1 because "
        "%select{it is not in module %2|the struct was imported from C}3",
        (Type, bool, Identifier, bool))
NOTE(designated_init_c_struct_fix,NoneType,
     "use \"self.init()\" to initialize the struct with zero values", ())


// Control flow diagnostics.
ERROR(missing_return,NoneType,
      "missing return in a %select{function|closure}1 expected to return %0",
      (Type, unsigned))
ERROR(missing_return_last_expr,NoneType,
      "missing return in a %select{function|closure}1 expected to return %0; "
      "did you mean to return the last expression?",
      (Type, unsigned))
ERROR(missing_never_call,NoneType,
      "%select{function|closure}1 with uninhabited return type %0 is missing "
      "call to another never-returning function on all paths",
      (Type, unsigned))
ERROR(guard_body_must_not_fallthrough,NoneType,
      "'guard' body must not fall through, consider using a 'return' or 'throw'"
      " to exit the scope", ())
WARNING(unreachable_code,NoneType, "will never be executed", ())
NOTE(unreachable_code_uninhabited_param_note,NoneType,
   "'%0' is uninhabited, so this function body can never be executed", (StringRef))
NOTE(unreachable_code_branch,NoneType,
     "condition always evaluates to %select{false|true}0", (bool))
NOTE(call_to_noreturn_note,NoneType,
     "a call to a never-returning function", ())
WARNING(unreachable_code_after_stmt,NoneType,
        "code after '%select{return|break|continue|throw}0' will never "
        "be executed", (unsigned))
WARNING(unreachable_case,NoneType,
        "%select{case|default}0 will never be executed", (bool))
WARNING(switch_on_a_constant,NoneType,
        "switch condition evaluates to a constant", ())
NOTE(unreachable_code_note,NoneType, "will never be executed", ())
WARNING(warn_infinite_recursive_function,NoneType,
        "all paths through this function will call itself", ())

// 'transparent' diagnostics
ERROR(circular_transparent,NoneType,
      "inlining 'transparent' functions forms circular loop", ())
NOTE(note_while_inlining,NoneType,
     "while inlining here", ())

// Pre-specializations
ERROR(cannot_prespecialize,NoneType,
      "Cannot pre-specialize %0", (StringRef))
ERROR(missing_prespecialization,NoneType,
      "Pre-specialized function %0 missing in SwiftOnoneSupport module",
      (StringRef))

// Arithmetic diagnostics.
ERROR(integer_conversion_overflow,NoneType,
      "integer overflows when converted from %0 to %1",
      (Type, Type))
ERROR(integer_conversion_overflow_builtin_types,NoneType,
      "integer overflows when converted from %select{unsigned|signed}0 "
      "%1 to %select{unsigned|signed}2 %3",
      (bool, Type, bool, Type))
WARNING(integer_conversion_overflow_warn,NoneType,
      "integer overflows when converted from %0 to %1",
      (Type, Type))
ERROR(integer_conversion_sign_error,NoneType,
      "negative integer cannot be converted to unsigned type %0",
      (Type))
ERROR(negative_integer_literal_overflow_unsigned,NoneType,
      "negative integer '%1' overflows when stored into unsigned type %0",
      (Type, StringRef))

ERROR(integer_literal_overflow,NoneType,
      "integer literal '%1' overflows when stored into %0",
      (Type, StringRef))
ERROR(integer_literal_overflow_builtin_types,NoneType,
      "integer literal '%2' overflows when stored into "
      "%select{unsigned|signed}0 %1", (bool, Type, StringRef))
WARNING(integer_literal_overflow_warn,NoneType,
      "integer literal overflows when stored into %0",
      (Type))
ERROR(arithmetic_operation_overflow,NoneType,
      "arithmetic operation '%0 %1 %2' (on type %3) results in an overflow",
      (StringRef, StringRef, StringRef, Type))
ERROR(arithmetic_operation_overflow_generic_type,NoneType,
      "arithmetic operation '%0 %1 %2' (on %select{unsigned|signed}3 "
      "%4-bit integer type) results in an overflow",
      (StringRef, StringRef, StringRef, bool, unsigned))
ERROR(division_overflow,NoneType,
      "division '%0 %1 %2' results in an overflow",
      (StringRef, StringRef, StringRef))
ERROR(division_by_zero,NoneType, "division by zero", ())
ERROR(wrong_non_negative_assumption,NoneType,
      "assumed non-negative value '%0' is negative", (StringRef))
ERROR(shifting_all_significant_bits,NoneType,
      "shift amount is greater than or equal to type size in bits", ())

// FIXME: We won't need this as it will be replaced with user-generated strings.
// staticReport diagnostics.
ERROR(static_report_error, NoneType,
      "static report error", ())

ERROR(pound_assert_condition_not_constant,NoneType,
       "#assert condition not constant", ())
ERROR(pound_assert_failure,NoneType,
       "%0", (StringRef))

NOTE(constexpr_unknown_reason_default,NoneType, "could not fold operation", ())
NOTE(constexpr_too_many_instructions,NoneType,
     "exceeded instruction limit: %0 when evaluating the expression at compile "
     "time", (unsigned))
NOTE(constexpr_loop,NoneType, "control flow loop found", ())
NOTE(constexpr_overflow,NoneType, "integer overflow detected", ())
NOTE(constexpr_trap,NoneType, "trap detected", ())
NOTE(constexpr_called_from,NoneType, "when called from here", ())
NOTE(constexpr_not_evaluable,NoneType,
     "expression not evaluable as constant here", ())

ERROR(non_physical_addressof,NoneType,
      "addressof only works with purely physical lvalues; "
      "use `withUnsafePointer` or `withUnsafeBytes` unless you're implementing "
      "`withUnsafePointer` or `withUnsafeBytes`", ())
ERROR(non_borrowed_indirect_addressof,NoneType,
      "addressof only works with borrowable in-memory rvalues; "
      "use `withUnsafePointer` or `withUnsafeBytes` unless you're implementing "
      "`withUnsafePointer` or `withUnsafeBytes`", ())

REMARK(opt_remark_passed, NoneType, "%0", (StringRef))
REMARK(opt_remark_missed, NoneType, "%0", (StringRef))

// Float-point to integer conversions
ERROR(float_to_int_overflow, NoneType,
  "invalid%select{| implicit}2 conversion: '%0' overflows %1", (StringRef, Type, bool))

ERROR(negative_fp_literal_overflow_unsigned, NoneType,
  "negative literal '%0' cannot be converted to %select{|unsigned }2%1",
  (StringRef, Type, bool))

// Overflow and underflow warnings for floating-point truncation
WARNING(warning_float_trunc_overflow, NoneType,
  "'%0' overflows to %select{|-}2inf during conversion to %1",
  (StringRef, Type, bool))

WARNING(warning_float_trunc_underflow, NoneType,
  "'%0' underflows and loses precision during conversion to %1",
  (StringRef, Type, bool))

WARNING(warning_float_trunc_hex_inexact, NoneType,
  "'%0' loses precision during conversion to %1",
  (StringRef, Type, bool))

WARNING(warning_float_overflows_maxbuiltin, NoneType,
  "'%0' overflows to %select{|-}1inf because its magnitude exceeds "
  "the limits of a float literal", (StringRef, bool))

// Integer to floating-point conversions
WARNING(warning_int_to_fp_inexact, NoneType,
  "'%1' is not exactly representable as %0; it becomes '%2'",
  (Type, StringRef, StringRef))


// Yield usage errors
ERROR(return_before_yield, NoneType, "accessor must yield before returning",())

ERROR(multiple_yields, NoneType, "accessor must not yield more than once", ())

NOTE(previous_yield, NoneType, "previous yield was here", ())

ERROR(possible_return_before_yield, NoneType,
      "accessor must yield on all paths before returning", ())

NOTE(branch_doesnt_yield, NoneType,
     "missing yield when the condition is %select{false|true}0", (bool))

NOTE(named_case_doesnt_yield, NoneType, "missing yield in the %0 case",
    (Identifier))

NOTE(case_doesnt_yield, NoneType, "missing yield in "
     "%select{this|the nil|the non-nil}0 case", (unsigned))

NOTE(switch_value_case_doesnt_yield, NoneType, "missing yield in the %0 case",
    (StringRef))

NOTE(try_branch_doesnt_yield, NoneType, "missing yield when error is "
     "%select{not |}0thrown", (bool))

#ifndef DIAG_NO_UNDEF
# if defined(DIAG)
#  undef DIAG
# endif
# undef REMARK
# undef NOTE
# undef WARNING
# undef ERROR
#endif
