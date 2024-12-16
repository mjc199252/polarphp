//===--- Config.h - Swift Language Platform Configuration -------*- C++ -*-===//
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
// Definitions of common interest in Swift.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_RUNTIME_CONFIG_H
#define POLARPHP_RUNTIME_CONFIG_H

#define POLAR_BNI_OS_BUILD 1

/// \macro POLAR_RUNTIME_GNUC_PREREQ
/// Extend the default __GNUC_PREREQ even if glibc's features.h isn't
/// available.
#ifndef POLAR_RUNTIME_GNUC_PREREQ
# if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
#  define POLAR_RUNTIME_GNUC_PREREQ(maj, min, patch) \
    ((__GNUC__ << 20) + (__GNUC_MINOR__ << 10) + __GNUC_PATCHLEVEL__ >= \
     ((maj) << 20) + ((min) << 10) + (patch))
# elif defined(__GNUC__) && defined(__GNUC_MINOR__)
#  define POLAR_RUNTIME_GNUC_PREREQ(maj, min, patch) \
    ((__GNUC__ << 20) + (__GNUC_MINOR__ << 10) >= ((maj) << 20) + ((min) << 10))
# else
#  define POLAR_RUNTIME_GNUC_PREREQ(maj, min, patch) 0
# endif
#endif

/// POLAR_RUNTIME_LIBRARY_VISIBILITY - If a class marked with this attribute is
/// linked into a shared library, then the class should be private to the
/// library and not accessible from outside it.  Can also be used to mark
/// variables and functions, making them private to any shared library they are
/// linked into.
/// On PE/COFF targets, library visibility is the default, so this isn't needed.
#if (__has_attribute(visibility) || POLAR_RUNTIME_GNUC_PREREQ(4, 0, 0)) &&    \
    !defined(__MINGW32__) && !defined(__CYGWIN__) && !defined(_WIN32)
#define POLAR_RUNTIME_LIBRARY_VISIBILITY __attribute__ ((visibility("hidden")))
#else
#define POLAR_RUNTIME_LIBRARY_VISIBILITY
#endif

/// Attributes.
/// POLAR_RUNTIME_ATTRIBUTE_NOINLINE - On compilers where we have a directive to do so,
/// mark a method "not for inlining".
#if __has_attribute(noinline) || POLAR_RUNTIME_GNUC_PREREQ(3, 4, 0)
#define POLAR_RUNTIME_ATTRIBUTE_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define POLAR_RUNTIME_ATTRIBUTE_NOINLINE __declspec(noinline)
#else
#define POLAR_RUNTIME_ATTRIBUTE_NOINLINE
#endif

/// POLAR_RUNTIME_ATTRIBUTE_ALWAYS_INLINE - On compilers where we have a directive to do
/// so, mark a method "always inline" because it is performance sensitive. GCC
/// 3.4 supported this but is buggy in various cases and produces unimplemented
/// errors, just use it in GCC 4.0 and later.
#if __has_attribute(always_inline) || POLAR_RUNTIME_GNUC_PREREQ(4, 0, 0)
#define POLAR_RUNTIME_ATTRIBUTE_ALWAYS_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
#define POLAR_RUNTIME_ATTRIBUTE_ALWAYS_INLINE __forceinline
#else
#define POLAR_RUNTIME_ATTRIBUTE_ALWAYS_INLINE
#endif

#ifdef __GNUC__
#define POLAR_RUNTIME_ATTRIBUTE_NORETURN __attribute__((noreturn))
#elif defined(_MSC_VER)
#define POLAR_RUNTIME_ATTRIBUTE_NORETURN __declspec(noreturn)
#else
#define POLAR_RUNTIME_ATTRIBUTE_NORETURN
#endif

/// POLAR_RUNTIME_BUILTIN_TRAP - On compilers which support it, expands to an expression
/// which causes the program to exit abnormally.
#if __has_builtin(__builtin_trap) || POLAR_RUNTIME_GNUC_PREREQ(4, 3, 0)
# define POLAR_RUNTIME_BUILTIN_TRAP __builtin_trap()
#elif defined(_MSC_VER)
// The __debugbreak intrinsic is supported by MSVC, does not require forward
// declarations involving platform-specific typedefs (unlike RaiseException),
// results in a call to vectored exception handlers, and encodes to a short
// instruction that still causes the trapping behavior we want.
# define POLAR_RUNTIME_BUILTIN_TRAP __debugbreak()
#else
# define POLAR_RUNTIME_BUILTIN_TRAP *(volatile int*)0x11 = 0
#endif

/// Does the current Polarphp platform support "unbridged" interoperation
/// with Objective-C?  If so, the implementations of various types must
/// implicitly handle Objective-C pointers.
///
/// Apple platforms support this by default.
#ifndef POLAR_OBJC_INTEROP
#ifdef __APPLE__
#define POLAR_OBJC_INTEROP 1
#else
#define POLAR_OBJC_INTEROP 0
#endif
#endif

/// Does the current Swift platform allow information other than the
/// class pointer to be stored in the isa field?  If so, when deriving
/// the class pointer of an object, we must apply a
/// dynamically-determined mask to the value loaded from the first
/// field of the object.
///
/// According to the Objective-C ABI, this is true only for 64-bit
/// platforms.
#ifndef POLAR_HAS_ISA_MASKING
#if POLAR_OBJC_INTEROP && __POINTER_WIDTH__ == 64
#define POLAR_HAS_ISA_MASKING 1
#else
#define POLAR_HAS_ISA_MASKING 0
#endif
#endif

/// Does the current Polarphp platform have ISA pointers which should be opaque
/// to anyone outside the Swift runtime?  Similarly to the ISA_MASKING case
/// above, information other than the class pointer could be contained in the
/// ISA.
#ifndef POLAR_HAS_OPAQUE_ISAS
#if defined(__arm__) && __ARM_ARCH_7K__ >= 2
#define POLAR_HAS_OPAQUE_ISAS 1
#else
#define POLAR_HAS_OPAQUE_ISAS 0
#endif
#endif

#if POLAR_HAS_OPAQUE_ISAS && POLAR_HAS_ISA_MASKING
#error Masking ISAs are incompatible with opaque ISAs
#endif

/// Which bits in the class metadata are used to distinguish Swift classes
/// from ObjC classes?
#ifndef POLAR_CLASS_IS_POLAR_MASK

// Non-Apple platforms always use 1.
# if !defined(__APPLE__)
#  define POLAR_CLASS_IS_POLAR_MASK 1ULL

// Builds for Swift-in-the-OS always use 2.
# elif POLAR_BNI_OS_BUILD
#  define POLAR_CLASS_IS_POLAR_MASK 2ULL

// Builds for Xcode always use 1.
# elif POLAR_BNI_XCODE_BUILD
#  define POLAR_CLASS_IS_POLAR_MASK 1ULL

// Other builds (such as local builds on developers' computers)
// dynamically choose the bit at runtime based on the current OS
// version.
# else
#  define POLAR_CLASS_IS_POLAR_MASK _swift_classIsSwiftMask
#  define POLAR_CLASS_IS_POLAR_MASK_GLOBAL_VARIABLE 1
#  define POLAR_BUILD_HAS_BACK_DEPLOYMENT 1
#  include "polarphp/runtime/BackDeployment.h"

# endif
#endif

// We try to avoid global constructors in the runtime as much as possible.
// These macros delimit allowed global ctors.
#if __clang__
# define POLAR_ALLOWED_RUNTIME_GLOBAL_CTOR_BEGIN \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wglobal-constructors\"")
# define POLAR_ALLOWED_RUNTIME_GLOBAL_CTOR_END \
    _Pragma("clang diagnostic pop")
#else
# define POLAR_ALLOWED_RUNTIME_GLOBAL_CTOR_BEGIN
# define POLAR_ALLOWED_RUNTIME_GLOBAL_CTOR_END
#endif

// Bring in visibility attribute macros
#include "polarphp/abi/shims/Visibility.h"

// Define mappings for calling conventions.

// Annotation for specifying a calling convention of
// a runtime function. It should be used with declarations
// of runtime functions like this:
// void runtime_function_name() SWIFT_CC(swift)
#define SWIFT_CC(CC) SWIFT_CC_##CC

// SWIFT_CC(c) is the C calling convention.
#define SWIFT_CC_c

// SWIFT_CC(swift) is the Swift calling convention.
// FIXME: the next comment is false.
// Functions outside the stdlib or runtime that include this file may be built
// with a compiler that doesn't support swiftcall; don't define these macros
// in that case so any incorrect usage is caught.
#if __has_attribute(swiftcall)
#define SWIFT_CC_swift __attribute__((swiftcall))
#define SWIFT_CONTEXT __attribute__((swift_context))
#define SWIFT_ERROR_RESULT __attribute__((swift_error_result))
#define SWIFT_INDIRECT_RESULT __attribute__((swift_indirect_result))
#else
#define SWIFT_CC_swift
#define SWIFT_CONTEXT
#define SWIFT_ERROR_RESULT
#define SWIFT_INDIRECT_RESULT
#endif

// SWIFT_CC(PreserveMost) is used in the runtime implementation to prevent
// register spills on the hot path.
// It is not safe to use for external calls; the loader's lazy function
// binding may not save all of the registers required for this convention.
#if __has_attribute(preserve_most) &&                                          \
    (defined(__aarch64__) || defined(__x86_64__))
#define POLAR_CC_PreserveMost __attribute__((preserve_most))
#else
#define POLAR_CC_PreserveMost
#endif

// This is the DefaultCC value used by the compiler.
// FIXME: the runtime's code does not honor DefaultCC
// so changing this value is not sufficient.
#define POLAR_DEFAULT_LLVM_CC llvm::CallingConv::C

#endif // POLARPHP_RUNTIME_CONFIG_H
