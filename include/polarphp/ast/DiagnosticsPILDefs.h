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
//
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
ERROR(bridging_module_missing,none,
      "unable to find module '%0' for implicit conversion function '%0.%1'",
      (StringRef, StringRef))
ERROR(bridging_function_missing,none,
      "unable to find implicit conversion function '%0.%1'",
      (StringRef, StringRef))
ERROR(bridging_function_overloaded,none,
      "multiple definitions of implicit conversion function '%0.%1'",
      (StringRef, StringRef))
ERROR(bridging_function_not_function,none,
      "definition of implicit conversion function '%0.%1' is not a function",
      (StringRef, StringRef))
ERROR(bridging_function_not_correct_type,none,
      "definition of implicit conversion function '%0.%1' is not of the correct"
      " type",
      (StringRef, StringRef))
ERROR(bridging_objcbridgeable_missing,none,
      "cannot find definition of '_ObjectiveCBridgeable' protocol", ())
ERROR(bridging_objcbridgeable_broken,none,
      "broken definition of '_ObjectiveCBridgeable' protocol: missing %0",
      (DeclName))

ERROR(invalid_sil_builtin,none,
      "INTERNAL ERROR: invalid use of builtin: %0",
      (StringRef))
ERROR(could_not_find_bridge_type,none,
      "could not find Objective-C bridge type for type %0; "
      "did you forget to import Foundation?", (Type))
ERROR(could_not_find_pointer_pointee_property,none,
      "could not find 'pointee' property of pointer type %0", (Type))

ERROR(writeback_overlap_property,none,
      "inout writeback to computed property %0 occurs in multiple arguments to"
      " call, introducing invalid aliasing", (Identifier))
ERROR(writeback_overlap_subscript,none,
      "inout writeback through subscript occurs in multiple arguments to call,"
      " introducing invalid aliasing",
      ())
NOTE(writebackoverlap_note,none,
      "concurrent writeback occurred here", ())

ERROR(inout_argument_alias,none,
      "inout arguments are not allowed to alias each other", ())
NOTE(previous_inout_alias,none,
      "previous aliasing argument", ())

ERROR(unimplemented_generator_witnesses,none,
      "protocol conformance emission for generator coroutines is unimplemented",
      ())

ERROR(exclusivity_access_required,none,
      "overlapping accesses to %0, but "
      "%select{initialization|read|modification|deinitialization}1 requires "
      "exclusive access; "
      "%select{consider copying to a local variable|"
              "consider calling MutableCollection.swapAt(_:_:)}2",
      (StringRef, unsigned, bool))

ERROR(exclusivity_access_required_unknown_decl,none,
        "overlapping accesses, but "
        "%select{initialization|read|modification|deinitialization}0 requires "
        "exclusive access; consider copying to a local variable", (unsigned))

NOTE(exclusivity_conflicting_access,none,
     "conflicting access is here", ())

ERROR(unsupported_c_function_pointer_conversion,none,
      "C function pointer signature %0 is not compatible with expected type %1",
      (Type, Type))

ERROR(c_function_pointer_from_function_with_context,none,
      "a C function pointer cannot be formed from a "
      "%select{local function|closure}0 that captures "
      "%select{context|generic parameters|dynamic Self type}1",
      (bool, unsigned))

ERROR(objc_selector_malformed,none,"the type ObjectiveC.Selector is malformed",
      ())

// Capture before declaration diagnostics.
ERROR(capture_before_declaration,none,
      "closure captures %0 before it is declared", (Identifier))
ERROR(capture_before_declaration_defer,none,
      "'defer' block captures %0 before it is declared", (Identifier))
NOTE(captured_value_declared_here,none,
     "captured value declared here", ())

#define SELECT_ESCAPING_CLOSURE_KIND "escaping %select{local function|closure}0"

// Invalid escaping capture diagnostics.
ERROR(escaping_inout_capture,none,
      SELECT_ESCAPING_CLOSURE_KIND
      " captures 'inout' parameter %1",
      (unsigned, Identifier))
NOTE(inout_param_defined_here,none,
     "parameter %0 is declared 'inout'", (Identifier))
ERROR(escaping_mutable_self_capture,none,
      SELECT_ESCAPING_CLOSURE_KIND
      " captures mutating 'self' parameter", (unsigned))

ERROR(escaping_noescape_param_capture,none,
      SELECT_ESCAPING_CLOSURE_KIND
      " captures non-escaping parameter %1", (unsigned, Identifier))
NOTE(noescape_param_defined_here,none,
     "parameter %0 is implicitly non-escaping", (Identifier))

ERROR(escaping_noescape_var_capture,none,
      SELECT_ESCAPING_CLOSURE_KIND
      " captures non-escaping value", (unsigned))

NOTE(value_captured_here,none,"captured here", ())

#undef SELECT_ESCAPING_CLOSURE_KIND

NOTE(value_captured_transitively,none,
     "captured indirectly by this call", ())

ERROR(err_noescape_param_call,none,
      "passing a %select{|closure which captures a }1non-escaping function "
      "parameter %0 to a call to a non-escaping function parameter can allow "
      "re-entrant modification of a variable",
      (DeclName, unsigned))

// Definite initialization diagnostics.
NOTE(variable_defined_here,none,
     "%select{variable|constant}0 defined here", (bool))
ERROR(variable_used_before_initialized,none,
      "%select{variable|constant}1 '%0' used before being initialized",
      (StringRef, bool))
ERROR(variable_inout_before_initialized,none,
      "%select{variable|constant}1 '%0' passed by reference before being"
      " initialized", (StringRef, bool))
ERROR(variable_closure_use_uninit,none,
      "%select{variable|constant}1 '%0' captured by a closure before being"
      " initialized", (StringRef, bool))
ERROR(variable_defer_use_uninit,none,
      "%select{variable|constant}1 '%0' used in defer before being"
      " initialized", (StringRef, bool))
ERROR(self_closure_use_uninit,none,
      "'self' captured by a closure before all members were initialized", ())


ERROR(variable_addrtaken_before_initialized,none,
      "address of %select{variable|constant}1 '%0' taken before it is"
      " initialized", (StringRef, bool))
ERROR(ivar_not_initialized_at_superinit,none,
      "property '%0' not initialized at super.init call", (StringRef, bool))
ERROR(ivar_not_initialized_at_implicit_superinit,none,
      "property '%0' not initialized at implicitly generated super.init call",
      (StringRef, bool))

ERROR(self_use_before_fully_init,none,
      "'self' used in %select{method call|property access}1 %0 before "
      "%select{all stored properties are initialized|"
      "'super.init' call|"
      "'self.init' call}2", (DeclBaseName, bool, unsigned))
ERROR(use_of_self_before_fully_init,none,
      "'self' used before all stored properties are initialized", ())


NOTE(stored_property_not_initialized,none,
     "'%0' not initialized", (StringRef))

ERROR(selfinit_multiple_times,none,
      "'%select{super|self}0.init' called multiple times in initializer",
      (unsigned))
ERROR(superselfinit_not_called_before_return,none,
      "'%select{super|self}0.init' isn't called on all paths before returning "
      "from initializer", (unsigned))
ERROR(self_before_superinit,none,
      "'self' used before 'super.init' call", ())
ERROR(self_before_selfinit,none,
      "'self' used before 'self.init' call", ())
ERROR(self_before_selfinit_value_type,none,
      "'self' used before 'self.init' call or assignment to 'self'", ())
ERROR(self_inside_catch_superselfinit,none,
      "'self' used inside 'catch' block reachable from "
      "%select{super|self}0.init call",
      (unsigned))
ERROR(return_from_init_without_initing_stored_properties,none,
      "return from initializer without initializing all"
      " stored properties", ())

ERROR(variable_function_use_uninit,none,
      "%select{variable|constant}1 '%0' used by function definition before"
      " being initialized",
      (StringRef, bool))
ERROR(struct_not_fully_initialized,none,
      "struct '%0' must be completely initialized before a member is stored to",
      (StringRef, bool))
ERROR(immutable_property_already_initialized,none,
      "immutable value '%0' may only be initialized once",
      (StringRef))
NOTE(initial_value_provided_in_let_decl,none,
     "initial value already provided in 'let' declaration", ())
ERROR(mutation_of_property_of_immutable_value,none,
      "cannot mutate %select{property %0|subscript}1 of immutable value '%2'",
      (DeclBaseName, bool, StringRef))
ERROR(using_mutating_accessor_on_immutable_value,none,
      "mutating accessor for %select{property %0|subscript}1 may not"
      " be used on immutable value '%2'",
      (DeclBaseName, bool, StringRef))
ERROR(mutating_method_called_on_immutable_value,none,
      "mutating %select{method|operator}1 %0 may not"
      " be used on immutable value '%2'",
      (DeclBaseName, unsigned, StringRef))
ERROR(immutable_value_passed_inout,none,
      "immutable value '%0' must not be passed inout",
      (StringRef))
ERROR(assignment_to_immutable_value,none,
      "immutable value '%0' must not be assigned to",
      (StringRef))

WARNING(designated_init_in_cross_module_extension,none,
        "initializer for struct %0 must use \"self.init(...)\" or \"self = ...\""
        "%select{| on all paths}1 because "
        "%select{it is not in module %2|the struct was imported from C}3",
        (Type, bool, Identifier, bool))
NOTE(designated_init_c_struct_fix,none,
     "use \"self.init()\" to initialize the struct with zero values", ())


// Control flow diagnostics.
ERROR(missing_return,none,
      "missing return in a %select{function|closure}1 expected to return %0",
      (Type, unsigned))
ERROR(missing_return_last_expr,none,
      "missing return in a %select{function|closure}1 expected to return %0; "
      "did you mean to return the last expression?",
      (Type, unsigned))
ERROR(missing_never_call,none,
      "%select{function|closure}1 with uninhabited return type %0 is missing "
      "call to another never-returning function on all paths",
      (Type, unsigned))
ERROR(guard_body_must_not_fallthrough,none,
      "'guard' body must not fall through, consider using a 'return' or 'throw'"
      " to exit the scope", ())
WARNING(unreachable_code,none, "will never be executed", ())
NOTE(unreachable_code_uninhabited_param_note,none,
	"'%0' is uninhabited, so this function body can never be executed", (StringRef))
NOTE(unreachable_code_branch,none,
     "condition always evaluates to %select{false|true}0", (bool))
NOTE(call_to_noreturn_note,none,
     "a call to a never-returning function", ())
WARNING(unreachable_code_after_stmt,none,
        "code after '%select{return|break|continue|throw}0' will never "
        "be executed", (unsigned))
WARNING(unreachable_case,none,
        "%select{case|default}0 will never be executed", (bool))
WARNING(switch_on_a_constant,none,
        "switch condition evaluates to a constant", ())
NOTE(unreachable_code_note,none, "will never be executed", ())
WARNING(warn_infinite_recursive_function,none,
        "all paths through this function will call itself", ())

// 'transparent' diagnostics
ERROR(circular_transparent,none,
      "inlining 'transparent' functions forms circular loop", ())
NOTE(note_while_inlining,none,
     "while inlining here", ())

// Pre-specializations
ERROR(cannot_prespecialize,none,
      "Cannot pre-specialize %0", (StringRef))
ERROR(missing_prespecialization,none,
      "Pre-specialized function %0 missing in SwiftOnoneSupport module",
      (StringRef))

// Arithmetic diagnostics.
ERROR(integer_conversion_overflow,none,
      "integer overflows when converted from %0 to %1",
      (Type, Type))
ERROR(integer_conversion_overflow_builtin_types,none,
      "integer overflows when converted from %select{unsigned|signed}0 "
      "%1 to %select{unsigned|signed}2 %3",
      (bool, Type, bool, Type))
WARNING(integer_conversion_overflow_warn,none,
      "integer overflows when converted from %0 to %1",
      (Type, Type))
ERROR(negative_integer_literal_overflow_unsigned,none,
      "negative integer '%1' overflows when stored into unsigned type %0",
      (Type, StringRef))

ERROR(integer_literal_overflow,none,
      "integer literal '%1' overflows when stored into %0",
      (Type, StringRef))
ERROR(integer_literal_overflow_builtin_types,none,
      "integer literal '%2' overflows when stored into "
      "%select{unsigned|signed}0 %1", (bool, Type, StringRef))
WARNING(integer_literal_overflow_warn,none,
      "integer literal overflows when stored into %0",
      (Type))
ERROR(arithmetic_operation_overflow,none,
      "arithmetic operation '%0 %1 %2' (on type %3) results in an overflow",
      (StringRef, StringRef, StringRef, Type))
ERROR(arithmetic_operation_overflow_generic_type,none,
      "arithmetic operation '%0 %1 %2' (on %select{unsigned|signed}3 "
      "%4-bit integer type) results in an overflow",
      (StringRef, StringRef, StringRef, bool, unsigned))
ERROR(division_overflow,none,
      "division '%0 %1 %2' results in an overflow",
      (StringRef, StringRef, StringRef))
ERROR(division_by_zero,none, "division by zero", ())
ERROR(wrong_non_negative_assumption,none,
      "assumed non-negative value '%0' is negative", (StringRef))
ERROR(shifting_all_significant_bits,none,
      "shift amount is greater than or equal to type size in bits", ())

// FIXME: We won't need this as it will be replaced with user-generated strings.
// staticReport diagnostics.
ERROR(static_report_error, none,
      "static report error", ())

ERROR(pound_assert_condition_not_constant,none,
       "#assert condition not constant", ())
ERROR(pound_assert_failure,none,
       "%0", (StringRef))

NOTE(constexpr_unknown_reason_default,none,
    "cannot evaluate expression as constant here", ())
NOTE(constexpr_unevaluable_operation,none,
    "cannot constant evaluate operation%select{| used by this call}0", (bool))

NOTE(constexpr_too_many_instructions,none,
     "exceeded instruction limit: %0 when evaluating the expression "
     "at compile time", (unsigned))
NOTE(constexpr_limit_exceeding_instruction,none, "limit exceeded "
     "%select{here|during this call}0", (bool))

NOTE(constexpr_loop_found_note,none,
    "control-flow loop found during evaluation ", ())
NOTE(constexpr_loop_instruction,none, "found loop "
    "%select{here|inside this call}0", (bool))

NOTE(constexpr_overflow,none, "integer overflow detected", ())
NOTE(constexpr_overflow_operation,none, "operation"
     "%select{| performed during this call}0 overflows", (bool))

NOTE(constexpr_trap, none, "%0", (StringRef))
NOTE(constexpr_trap_operation, none, "operation"
     "%select{| performed during this call}0 traps", (bool))

NOTE(constexpr_invalid_operand_seen, none,
    "operation with invalid operands encountered during evaluation",())
NOTE(constexpr_operand_invalid_here, none,
    "operation with invalid operands encountered "
    "%select{here|during this call}0", (bool))

NOTE(constexpr_value_unknown_at_top_level,none,
    "cannot evaluate top-level value as constant here",())
NOTE(constexpr_multiple_writers_found_at_top_level,none,
     "top-level value has multiple assignments",())

NOTE(constexpr_unsupported_instruction_found, none,
    "encountered operation not supported by the evaluator: %0", (StringRef))
NOTE(constexpr_unsupported_instruction_found_here,none, "operation"
     "%select{| used by this call is}0 not supported by the evaluator", (bool))

NOTE(constexpr_found_callee_with_no_body, none,
    "encountered call to '%0' whose body is not available. "
    "Imported functions must be marked '@inlinable' to constant evaluate",
    (StringRef))
NOTE(constexpr_callee_with_no_body, none,
    "%select{|calls a }0function whose body is not available", (bool))

NOTE(constexpr_found_call_with_unknown_arg, none,
    "encountered call to '%0' where the %1 argument is not a constant",
    (StringRef, StringRef))
NOTE(constexpr_call_with_unknown_arg, none,
    "%select{|makes a }0function call with non-constant arguments", (bool))

NOTE(constexpr_untracked_sil_value_use_found, none,
    "encountered use of a variable not tracked by the evaluator", ())
NOTE(constexpr_untracked_sil_value_used_here, none,
    "untracked variable used %select{here|by this call}0", (bool))

NOTE(constexpr_unresolvable_witness_call, none,
    "encountered unresolvable witness method call: '%0'", (StringRef))
NOTE(constexpr_no_witness_table_entry, none, "cannot find witness table entry "
    "%select{for this call|for a witness-method invoked during this call}0",
    (bool))
NOTE(constexpr_witness_call_with_no_conformance, none,
    "cannot find concrete conformance "
    "%select{for this call|for a witness-method invoked during this call}0",
    (bool))

NOTE(constexpr_unknown_control_flow_due_to_skip,none, "branch depends on "
     "non-constant value produced by an unevaluated instructions", ())
NOTE(constexpr_returned_by_unevaluated_instruction,none,
     "result of an unevaluated instruction is not a constant", ())
NOTE(constexpr_mutated_by_unevaluated_instruction,none, "value mutable by an "
    "unevaluated instruction is not a constant", ())

ERROR(not_constant_evaluable, none, "not constant evaluable", ())
ERROR(constexpr_imported_func_not_onone, none, "imported constant evaluable "
      "function '%0' must be annotated '@_optimize(none)'", (StringRef))

ERROR(non_physical_addressof,none,
      "addressof only works with purely physical lvalues; "
      "use `withUnsafePointer` or `withUnsafeBytes` unless you're implementing "
      "`withUnsafePointer` or `withUnsafeBytes`", ())
ERROR(non_borrowed_indirect_addressof,none,
      "addressof only works with borrowable in-memory rvalues; "
      "use `withUnsafePointer` or `withUnsafeBytes` unless you're implementing "
      "`withUnsafePointer` or `withUnsafeBytes`", ())

REMARK(opt_remark_passed, none, "%0", (StringRef))
REMARK(opt_remark_missed, none, "%0", (StringRef))

// Float-point to integer conversions
ERROR(float_to_int_overflow, none,
  "invalid%select{| implicit}2 conversion: '%0' overflows %1", (StringRef, Type, bool))

ERROR(negative_fp_literal_overflow_unsigned, none,
  "negative literal '%0' cannot be converted to %select{|unsigned }2%1",
  (StringRef, Type, bool))

// Overflow and underflow warnings for floating-point truncation
WARNING(warning_float_trunc_overflow, none,
  "'%0' overflows to %select{|-}2inf during conversion to %1",
  (StringRef, Type, bool))

WARNING(warning_float_trunc_underflow, none,
  "'%0' underflows and loses precision during conversion to %1",
  (StringRef, Type, bool))

WARNING(warning_float_trunc_hex_inexact, none,
  "'%0' loses precision during conversion to %1",
  (StringRef, Type, bool))

WARNING(warning_float_overflows_maxbuiltin, none,
  "'%0' overflows to %select{|-}1inf because its magnitude exceeds "
  "the limits of a float literal", (StringRef, bool))

// Integer to floating-point conversions
WARNING(warning_int_to_fp_inexact, none,
  "'%1' is not exactly representable as %0; it becomes '%2'",
  (Type, StringRef, StringRef))


// Yield usage errors
ERROR(return_before_yield, none, "accessor must yield before returning",())

ERROR(multiple_yields, none, "accessor must not yield more than once", ())

NOTE(previous_yield, none, "previous yield was here", ())

ERROR(possible_return_before_yield, none,
      "accessor must yield on all paths before returning", ())

NOTE(branch_doesnt_yield, none,
     "missing yield when the condition is %select{false|true}0", (bool))

NOTE(named_case_doesnt_yield, none, "missing yield in the %0 case",
    (Identifier))

NOTE(case_doesnt_yield, none, "missing yield in "
     "%select{this|the nil|the non-nil}0 case", (unsigned))

NOTE(switch_value_case_doesnt_yield, none, "missing yield in the %0 case",
    (StringRef))

NOTE(try_branch_doesnt_yield, none, "missing yield when error is "
     "%select{not |}0thrown", (bool))

// OS log optimization diagnostics.

ERROR(oslog_non_const_interpolation_options, none, "interpolation arguments "
      "like format and privacy options must be constants", ())

ERROR(oslog_const_evaluable_fun_error, none, "evaluation of constant-evaluable "
      "function '%0' failed", (StringRef))

ERROR(oslog_fail_stop_error, none, "constant evaluation of log call failed "
      "with fatal error", ())

ERROR(oslog_non_constant_message, none, "'OSLogMessage' instance passed to the "
      "log call is not a constant", ())

ERROR(oslog_non_constant_interpolation, none, "'OSLogInterpolation' instance "
      "passed to 'OSLogMessage.init' is not a constant", ())

ERROR(oslog_property_not_constant, none, "'OSLogInterpolation.%0' is not a "
      "constant", (StringRef))

ERROR(global_string_pointer_on_non_constant, none, "globalStringTablePointer "
      "builtin must used only on string literals", ())

ERROR(polymorphic_builtin_passed_non_trivial_non_builtin_type, none, "Argument "
      "of type %0 can not be passed as an argument to a Polymorphic "
      "builtin. Polymorphic builtins can only be passed arguments that are "
      "trivial builtin typed", (Type))

ERROR(polymorphic_builtin_passed_type_without_static_overload, none, "Static"
      " overload %0 does not exist for polymorphic builtin '%1'. Static "
      "overload implied by passing argument of type %2",
      (Identifier, StringRef, Type))

ERROR(box_to_stack_cannot_promote_box_to_stack_due_to_escape_alloc, none,
      "Can not promote value from heap to stack due to value escaping", ())
NOTE(box_to_stack_cannot_promote_box_to_stack_due_to_escape_location, none,
     "value escapes here", ())

#ifndef DIAG_NO_UNDEF
# if defined(DIAG)
#  undef DIAG
# endif
# undef REMARK
# undef NOTE
# undef WARNING
# undef ERROR
#endif
