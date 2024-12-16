//===--- Debug.h - Swift Runtime debug helpers ------------------*- C++ -*-===//
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
// Random debug support
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_RUNTIME_DEBUG_HELPERS_H
#define POLARPHP_RUNTIME_DEBUG_HELPERS_H

#include <cstdarg>
#include <cstdio>
#include <stdint.h>
#include "polarphp/runtime/Config.h"
#include "polarphp/runtime/Unreachable.h"

#ifdef POLAR_HAVE_CRASHREPORTERCLIENT

#define CRASHREPORTER_ANNOTATIONS_VERSION 5
#define CRASHREPORTER_ANNOTATIONS_SECTION "__crash_info"

struct crashreporter_annotations_t {
  uint64_t version;          // unsigned long
  uint64_t message;          // char *
  uint64_t signature_string; // char *
  uint64_t backtrace;        // char *
  uint64_t message2;         // char *
  uint64_t thread;           // uint64_t
  uint64_t dialog_mode;      // unsigned int
  uint64_t abort_cause;      // unsigned int
};

extern "C" {
POLAR_RUNTIME_LIBRARY_VISIBILITY
extern struct crashreporter_annotations_t gCRAnnotations;
}

POLAR_RUNTIME_ATTRIBUTE_ALWAYS_INLINE
static inline void CRSetCrashLogMessage(const char *message) {
  gCRAnnotations.message = reinterpret_cast<uint64_t>(message);
}

POLAR_RUNTIME_ATTRIBUTE_ALWAYS_INLINE
static inline const char *CRGetCrashLogMessage() {
  return reinterpret_cast<const char *>(gCRAnnotations.message);
}

#else

POLAR_RUNTIME_ATTRIBUTE_ALWAYS_INLINE
static inline void CRSetCrashLogMessage(const char *) {}

#endif

namespace polar::runtime {

// Duplicated from Metadata.h. We want to use this header
// in places that cannot themselves include Metadata.h.
struct InProcess;
template <typename Runtime> struct TargetMetadata;
using Metadata = TargetMetadata<InProcess>;

// swift::crash() halts with a crash log message,
// but otherwise tries not to disturb register state.

POLAR_RUNTIME_ATTRIBUTE_NORETURN
POLAR_RUNTIME_ATTRIBUTE_ALWAYS_INLINE // Minimize trashed registers
static inline void crash(const char *message) {
   CRSetCrashLogMessage(message);

   POLAR_RUNTIME_BUILTIN_TRAP;
   polarphp_runtime_unreachable("Expected compiler to crash.");
}

// swift::fatalError() halts with a crash log message,
// but makes no attempt to preserve register state.
POLAR_RUNTIME_ATTRIBUTE_NORETURN
extern void
fatalError(uint32_t flags, const char *format, ...);

/// swift::warning() emits a warning from the runtime.
extern void
warningv(uint32_t flags, const char *format, va_list args);

/// swift::warning() emits a warning from the runtime.
extern void
warning(uint32_t flags, const char *format, ...);

// polarphp_dynamicCastFailure halts using fatalError()
// with a description of a failed cast's types.
POLAR_RUNTIME_ATTRIBUTE_NORETURN
void
polarphp_dynamicCastFailure(const Metadata *sourceType,
                            const Metadata *targetType,
                            const char *message = nullptr);

// polarphp_dynamicCastFailure halts using fatalError()
// with a description of a failed cast's types.
POLAR_RUNTIME_ATTRIBUTE_NORETURN
void
polarphp_dynamicCastFailure(const void *sourceType, const char *sourceName,
                            const void *targetType, const char *targetName,
                            const char *message = nullptr);

POLAR_RUNTIME_EXPORT
void polarphp_reportError(uint32_t flags, const char *message);

// Halt due to an overflow in polarphp_retain().
POLAR_RUNTIME_ATTRIBUTE_NORETURN POLAR_RUNTIME_ATTRIBUTE_NOINLINE
void polarphp_abortRetainOverflow();

// Halt due to reading an unowned reference to a dead object.
POLAR_RUNTIME_ATTRIBUTE_NORETURN POLAR_RUNTIME_ATTRIBUTE_NOINLINE
void polarphp_abortRetainUnowned(const void *object);

// Halt due to an overflow in polarphp_unownedRetain().
POLAR_RUNTIME_ATTRIBUTE_NORETURN POLAR_RUNTIME_ATTRIBUTE_NOINLINE
void polarphp_abortUnownedRetainOverflow();

// Halt due to an overflow in incrementWeak().
POLAR_RUNTIME_ATTRIBUTE_NORETURN POLAR_RUNTIME_ATTRIBUTE_NOINLINE
void polarphp_abortWeakRetainOverflow();

// Halt due to enabling an already enabled dynamic replacement().
POLAR_RUNTIME_ATTRIBUTE_NORETURN POLAR_RUNTIME_ATTRIBUTE_NOINLINE
void polarphp_abortDynamicReplacementEnabling();

// Halt due to disabling an already disabled dynamic replacement().
POLAR_RUNTIME_ATTRIBUTE_NORETURN POLAR_RUNTIME_ATTRIBUTE_NOINLINE
void polarphp_abortDynamicReplacementDisabling();

/// This function dumps one line of a stack trace. It is assumed that \p framePC
/// is the address of the stack frame at index \p index. If \p shortOutput is
/// true, this functions prints only the name of the symbol and offset, ignores
/// \p index argument and omits the newline.
void dumpStackTraceEntry(unsigned index, void *framePC,
                         bool shortOutput = false);

POLAR_RUNTIME_ATTRIBUTE_NOINLINE
void printCurrentBacktrace(unsigned framesToSkip = 1);

/// Debugger breakpoint ABI. This structure is passed to the debugger (and needs
/// to be stable) and describes extra information about a fatal error or a
/// non-fatal warning, which should be logged as a runtime issue. Please keep
/// all integer values pointer-sized.
struct RuntimeErrorDetails {
   static const uintptr_t currentVersion = 2;

   // ABI version, needs to be set to "currentVersion".
   uintptr_t version;

   // A short hyphenated string describing the type of the issue, e.g.
   // "precondition-failed" or "exclusivity-violation".
   const char *errorType;

   // Description of the current thread's stack position.
   const char *currentStackDescription;

   // Number of frames in the current stack that should be ignored when reporting
   // the issue (exluding the reportToDebugger/_polarphp_runtime_on_report frame).
   // The remaining top frame should point to user's code where the bug is.
   uintptr_t framesToSkip;

   // Address of some associated object (if there's any).
   const void *memoryAddress;

   // A structure describing an extra thread (and its stack) that is related.
   struct Thread {
      const char *description;
      uint64_t threadID;
      uintptr_t numFrames;
      void **frames;
   };

   // Number of extra threads (excluding the current thread) that are related,
   // and the pointer to the array of extra threads.
   uintptr_t numExtraThreads;
   Thread *threads;

   // Describes a suggested fix-it. Text in [startLine:startColumn,
   // endLine:endColumn) is to be replaced with replacementText.
   struct FixIt {
      const char *filename;
      uintptr_t startLine;
      uintptr_t startColumn;
      uintptr_t endLine;
      uintptr_t endColumn;
      const char *replacementText;
   };

   // Describes some extra information, possible with fix-its, about the current
   // runtime issue.
   struct Note {
      const char *description;
      uintptr_t numFixIts;
      FixIt *fixIts;
   };

   // Number of suggested fix-its, and the pointer to the array of them.
   uintptr_t numFixIts;
   FixIt *fixIts;

   // Number of related notes, and the pointer to the array of them.
   uintptr_t numNotes;
   Note *notes;
};

enum: uintptr_t {
   RuntimeErrorFlagNone = 0,
   RuntimeErrorFlagFatal = 1 << 0
};

/// Debugger hook. Calling this stops the debugger with a message and details
/// about the issues. Called by overlays.
POLAR_RUNTIME_STDLIB_SPI
void _polarphp_reportToDebugger(uintptr_t flags, const char *message,
                                RuntimeErrorDetails *details = nullptr);

POLAR_RUNTIME_STDLIB_SPI
bool _polarphp_reportFatalErrorsToDebugger;

POLAR_RUNTIME_STDLIB_SPI
bool _polarphp_shouldReportFatalErrorsToDebugger();


POLAR_RUNTIME_ATTRIBUTE_ALWAYS_INLINE
inline static int polarphp_asprintf(char **strp, const char *fmt, ...) {
   va_list args;
   va_start(args, fmt);
#if defined(_WIN32)
   #pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
  int len = _vscprintf(fmt, args);
#pragma GCC diagnostic pop
  if (len < 0) {
    va_end(args);
    return -1;
  }
  char *buffer = static_cast<char *>(malloc(len + 1));
  if (!buffer) {
    va_end(args);
    return -1;
  }
  int result = vsprintf(buffer, fmt, args);
  if (result < 0) {
    va_end(args);
    free(buffer);
    return -1;
  }
  *strp = buffer;
#else
   int result = vasprintf(strp, fmt, args);
#endif
   va_end(args);
   return result;
}

} // namespace polar::runtime

#endif // POLARPHP_RUNTIME_DEBUG_HELPERS_H
