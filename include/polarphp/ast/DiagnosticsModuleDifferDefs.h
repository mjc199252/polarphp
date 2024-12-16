//===--- DiagnosticsModuleDiffer.def - Diagnostics Text ---------*- C++ -*-===//
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
//  This file defines diagnostics emitted during diffing two Swift modules.
//  Each diagnostic is described using one of three kinds (error, warning, or
//  note) along with a unique identifier, category, options, and text, and is
//  followed by a signature describing the diagnostic argument kinds.
//
//===----------------------------------------------------------------------===//

#if !(defined(DIAG) || (defined(ERROR) && defined(WARNING) && defined(NOTE)))
#  error Must define either DIAG or the set {ERROR,WARNING,NOTE}
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

ERROR(generic_sig_change,none,"%0 has generic signature change from %1 to %2", (StringRef, StringRef, StringRef))

ERROR(raw_type_change,none,"%0(%1) is now %2 representable", (StringRef, StringRef, StringRef))

ERROR(removed_decl,none,"%0 has been removed%select{| (deprecated)}1", (StringRef, bool))

ERROR(moved_decl,none,"%0 has been moved to %1", (StringRef, StringRef))

ERROR(renamed_decl,none,"%0 has been renamed to %1", (StringRef, StringRef))

ERROR(decl_type_change,none,"%0 has %1 type change from %2 to %3", (StringRef, StringRef, StringRef, StringRef))

ERROR(decl_attr_change,none,"%0 changes from %1 to %2", (StringRef, StringRef, StringRef))

ERROR(decl_new_attr,none,"%0 is now %1", (StringRef, StringRef))

ERROR(decl_reorder,none,"%0 in a non-resilient type changes position from %1 to %2", (StringRef, unsigned, unsigned))

ERROR(decl_added,none,"%0 is added to a non-resilient type", (StringRef))

ERROR(var_has_fixed_order_change,none,"%0 is %select{now|no longer}1 a stored property", (StringRef, bool))

ERROR(func_has_fixed_order_change,none,"%0 is %select{now|no longer}1 a non-final instance function", (StringRef, bool))

ERROR(default_arg_removed,none,"%0 has removed default argument from %1", (StringRef, StringRef))

ERROR(conformance_removed,none,"%0 has removed %select{conformance to|inherited protocol}2 %1", (StringRef, StringRef, bool))

ERROR(conformance_added,none,"%0 has added inherited protocol %1", (StringRef, StringRef))

ERROR(existing_conformance_added,none,"%0 has added a conformance to an existing protocol %1", (StringRef, StringRef))

ERROR(default_associated_type_removed,none,"%0 has removed default type %1", (StringRef, StringRef))

ERROR(protocol_req_added,none,"%0 has been added as a protocol requirement", (StringRef))

ERROR(super_class_removed,none,"%0 has removed its super class %1", (StringRef, StringRef))

ERROR(super_class_changed,none,"%0 has changed its super class from %1 to %2", (StringRef, StringRef, StringRef))

ERROR(decl_kind_changed,none,"%0 has been changed to a %1", (StringRef, StringRef))

ERROR(optional_req_changed,none,"%0 is %select{now|no longer}1 an optional requirement", (StringRef, bool))

ERROR(no_longer_open,none,"%0 is no longer open for subclassing", (StringRef))

ERROR(func_type_escaping_changed,none,"%0 has %select{removed|added}2 @escaping in %1", (StringRef, StringRef, bool))

ERROR(func_self_access_change,none,"%0 has self access kind changing from %1 to %2", (StringRef, StringRef, StringRef))

ERROR(param_ownership_change,none,"%0 has %1 changing from %2 to %3", (StringRef, StringRef, StringRef, StringRef))

ERROR(type_witness_change,none,"%0 has type witness type for %1 changing from %2 to %3", (StringRef, StringRef, StringRef, StringRef))

ERROR(decl_new_witness_table_entry,none,"%0 now requires %select{|no}1 new witness table entry", (StringRef, bool))

ERROR(new_decl_without_intro,none,"%0 is a new API without @available attribute", (StringRef))

ERROR(objc_name_change,none,"%0 has ObjC name change from %1 to %2", (StringRef, StringRef, StringRef))

ERROR(desig_init_added,none,"%0 has been added as a designated initializer to an open class", (StringRef))

#ifndef DIAG_NO_UNDEF
# if defined(DIAG)
#  undef DIAG
# endif
# undef NOTE
# undef WARNING
# undef ERROR
#endif
