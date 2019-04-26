//===--- AttrKind.h - Enumerate attribute kinds  ----------------*- C++ -*-===//
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
// This file defines enumerations related to declaration attributes.
//
//===----------------------------------------------------------------------===//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/26.

#ifndef POLARPHP_AST_ATTR_KIND_H
#define POLARPHP_AST_ATTR_KIND_H

#include "polarphp/basic/InlineBitfield.h"

/// forward declare class with namespace
namespace polar::basic {
class StringRef;
}

namespace polar::ast {

using polar::basic::StringRef;

/// The associativity of a binary operator.
enum class Associativity : uint8_t
{
   /// Non-associative operators cannot be written next to other
   /// operators with the same precedence.  Relational operators are
   /// typically non-associative.
   None,

   /// Left-associative operators associate to the left if written next
   /// to other left-associative operators of the same precedence.
   Left,

   /// Right-associative operators associate to the right if written
   /// next to other right-associative operators of the same precedence.
   Right
};

/// Returns the in-source spelling of the given associativity.
StringRef get_associativity_spelling(Associativity value);

/// The kind of unary operator, if any.
enum class UnaryOperatorKind : uint8_t
{
   None,
   Prefix,
   Postfix
};

/// Access control levels.
// These are used in diagnostics and with < and similar operations,
// so please do not reorder existing values.
enum class AccessLevel : uint8_t
{
   /// Private access is limited to the current scope.
   Private = 0,
   /// File-private access is limited to the current file.
   FilePrivate,
   /// Internal access is limited to the current module.
   Internal,
   /// Public access is not limited, but some capabilities may be
   /// restricted outside of the current module.
   Public,
   /// Open access is not limited, and all capabilities are unrestricted.
   Open,
};

/// Returns the in-source spelling of the given access level.
StringRef get_access_level_spelling(AccessLevel value);

enum class InlineKind : uint8_t
{
   Never = 0,
   Always = 1,
   Last_InlineKind = Always
};

enum : unsigned {
   NumInlineKindBits =
                  polar::basic::count_bits_used(static_cast<unsigned>(InlineKind::Last_InlineKind))
};

/// This enum represents the possible values of the @_effects attribute.
/// These values are ordered from the strongest guarantee to the weakest,
/// so please do not reorder existing values.
enum class EffectsKind : uint8_t
{
   ReadNone,
   ReadOnly,
   ReleaseNone,
   ReadWrite,
   Unspecified,
   Last_EffectsKind = Unspecified
};

enum : unsigned {
   NumEffectsKindBits =
                  polar::basic::count_bits_used(static_cast<unsigned>(EffectsKind::Last_EffectsKind))
};

enum DeclAttrKind : unsigned {
#define DECL_ATTR(_, NAME, ...) DAK_##NAME,
#include "polarphp/ast/AttrDefs.h"
   DAK_Count
};

enum : unsigned
{
   NumDeclAttrKindBits =
                  polar::basic::count_bits_used(static_cast<unsigned>(DeclAttrKind::DAK_Count - 1))
};

// Define enumerators for each type attribute, e.g. TAK_weak.
enum TypeAttrKind
{
#define TYPE_ATTR(X) TAK_##X,
#include "polarphp/ast/AttrDefs.h"
   TAK_Count
};

} // polar::basic

#endif // POLARPHP_AST_ATTR_KIND_H