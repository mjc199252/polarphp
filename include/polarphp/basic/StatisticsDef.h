//===--- Statistics.def - Statistics Macro Metaprogramming Database -*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This file defines the database of always-available statistic counters.
//
// DRIVER_STATISTIC(Id)
//   - Id is an identifier suitable for use in C++
//
// FRONTEND_STATISTIC(Subsystem, Id)
//   - Subsystem is a token to be stringified as a name prefix
//   - Id is an identifier suitable for use in C++
//
//===----------------------------------------------------------------------===//

/// Driver statistics are collected for driver processes
#ifdef DRIVER_STATISTIC

/// Total number of jobs (frontend, merge-modules, link, etc.) run by the
/// driver.  This should be some number less than the total number of files in
/// the module in primary-files mode, and will likely just be 1 or 2 in WMO
/// mode.
DRIVER_STATISTIC(NumDriverJobsRun)

/// Total number of jobs (frontend, merge-modules, link, etc.) which _could_
/// have been run by the driver, but that it decided to skip due to analysis of
/// the dependency graph induced by .swiftdeps files. This, added together with
/// the number of driver jobs run (above) should be relatively constant
/// run-over-run.
DRIVER_STATISTIC(NumDriverJobsSkipped)

/// Total number of driver processes that exited with EXIT_FAILURE / not with
/// EXIT_SUCCESS.
DRIVER_STATISTIC(NumProcessFailures)

/// Total number of driver poll() calls on subprocess pipes.
DRIVER_STATISTIC(NumDriverPipePolls)

/// Total number of driver read() calls on subprocess pipes.
DRIVER_STATISTIC(NumDriverPipeReads)

/// Next 10 statistics count dirtying-events in the driver's dependency graph,
/// which it uses to decide which files are invalid (and thus which files to
/// build). There are two dimensions to each dirtying event:
///
///  - the type of dependency that caused the dirtying (top-level names, dynamic
///    lookups, nominal type usage, member usage, or external-files)
///
///  - whether the dependency should "cascade" (transitively dirty the node's
///    own downstream dependents, vs. just dirtying the node itself)
DRIVER_STATISTIC(DriverDepCascadingTopLevel)
DRIVER_STATISTIC(DriverDepCascadingDynamic)
DRIVER_STATISTIC(DriverDepCascadingNominal)
DRIVER_STATISTIC(DriverDepCascadingMember)
DRIVER_STATISTIC(DriverDepCascadingExternal)

DRIVER_STATISTIC(DriverDepTopLevel)
DRIVER_STATISTIC(DriverDepDynamic)
DRIVER_STATISTIC(DriverDepNominal)
DRIVER_STATISTIC(DriverDepMember)
DRIVER_STATISTIC(DriverDepExternal)

/// Maximum Resident Set Size (roughly: physical memory actually used) by the
/// tree of processes launched by the driver (i.e. the entire compilation).
DRIVER_STATISTIC(ChildrenMaxRSS)
#endif

/// Driver statistics are collected for frontend processes
#ifdef FRONTEND_STATISTIC

/// Total number of frontend processes that exited with EXIT_FAILURE / not with
/// EXIT_SUCCESS.
FRONTEND_STATISTIC(Frontend, NumProcessFailures)

/// Total instructions-executed count in each frontend process.
FRONTEND_STATISTIC(Frontend, NumInstructionsExecuted)

/// Maximum number of bytes allocated via malloc.
FRONTEND_STATISTIC(Frontend, MaxMallocUsage)

/// Number of source buffers visible in the source manager.
FRONTEND_STATISTIC(AST, NumSourceBuffers)

/// Total number of lines of source code (just by counting newlines) in all the
/// source buffers visible in the source manager. Crude proxy for "project
/// size".
FRONTEND_STATISTIC(AST, NumSourceLines)

/// The NumSourceLines value of a frontend divided by the user-time of the
/// frontend; stored and emitted separately so there's a precomputed value a
/// user can grep-for to find a slow frontend.
FRONTEND_STATISTIC(AST, NumSourceLinesPerSecond)

/// Number of libraries (including frameworks) linked against.
FRONTEND_STATISTIC(AST, NumLinkLibraries)

/// Number of top-level modules loaded in the AST context.
FRONTEND_STATISTIC(AST, NumLoadedModules)

/// Number of Clang entities imported into the AST context.
FRONTEND_STATISTIC(AST, NumTotalClangImportedEntities)

/// Number of bytes allocated in the AST's local arenas.
FRONTEND_STATISTIC(AST, NumAstBytesAllocated)

/// Number of file-level dependencies of this frontend job, as tracked in the
/// AST context's dependency collector.
FRONTEND_STATISTIC(AST, NumDependencies)

/// Number of top-level, dynamic, and member names referenced in this frontend
/// job's source file, as tracked by the AST context's referenced-name tracker.
FRONTEND_STATISTIC(AST, NumReferencedTopLevelNames)
FRONTEND_STATISTIC(AST, NumReferencedDynamicNames)
FRONTEND_STATISTIC(AST, NumReferencedMemberNames)

/// Number of declarations in the AST context.
FRONTEND_STATISTIC(AST, NumDecls)

/// Number of local type declarations in the AST context.
FRONTEND_STATISTIC(AST, NumLocalTypeDecls)

/// Number of Objective-C declarations in the AST context.
FRONTEND_STATISTIC(AST, NumObjCMethods)

/// Number of infix, postfix, and prefix operators in the AST context.
FRONTEND_STATISTIC(AST, NumInfixOperators)
FRONTEND_STATISTIC(AST, NumPostfixOperators)
FRONTEND_STATISTIC(AST, NumPrefixOperators)

/// Number of precedence groups in the AST context.
FRONTEND_STATISTIC(AST, NumPrecedenceGroups)

/// Number of qualified lookups into a nominal type.
FRONTEND_STATISTIC(AST, NumLookupQualifiedInNominal)

/// Number of qualified lookups into a module.
FRONTEND_STATISTIC(AST, NumLookupQualifiedInModule)

/// Number of qualified lookups into AnyObject.
FRONTEND_STATISTIC(AST, NumLookupQualifiedInAnyObject)

/// Number of lookups into a module and its imports.
FRONTEND_STATISTIC(AST, NumLookupInModule)

/// Number of local lookups into a module.
FRONTEND_STATISTIC(AST, NumModuleLookupValue)

/// Number of local lookups into a module's class members, for
/// AnyObject lookup.
FRONTEND_STATISTIC(AST, NumModuleLookupClassMember)

/// Number of body scopes for iterable types
FRONTEND_STATISTIC(AST, NumIterableTypeBodyAstScopes)

/// Number of expansions of body scopes for iterable types
FRONTEND_STATISTIC(AST, NumIterableTypeBodyAstScopeExpansions)

/// Number of brace statment scopes for iterable types
FRONTEND_STATISTIC(AST, NumBraceStmtAstScopes)

/// Number of expansions of brace statement scopes for iterable types
FRONTEND_STATISTIC(AST, NumBraceStmtAstScopeExpansions)

/// Number of ASTScope lookups
FRONTEND_STATISTIC(AST, NumAstScopeLookups)

/// Number of lookups of the cached import graph for a module or
/// source file.
FRONTEND_STATISTIC(AST, ImportSetFoldHit)
FRONTEND_STATISTIC(AST, ImportSetFoldMiss)

FRONTEND_STATISTIC(AST, ImportSetCacheHit)
FRONTEND_STATISTIC(AST, ImportSetCacheMiss)

FRONTEND_STATISTIC(AST, ModuleVisibilityCacheHit)
FRONTEND_STATISTIC(AST, ModuleVisibilityCacheMiss)

FRONTEND_STATISTIC(AST, ModuleShadowCacheHit)
FRONTEND_STATISTIC(AST, ModuleShadowCacheMiss)

/// Number of full function bodies parsed.
FRONTEND_STATISTIC(Parse, NumFunctionsParsed)

/// Number of full braced decl list parsed.
FRONTEND_STATISTIC(Parse, NumIterableDeclContextParsed)

#define POLAR_REQUEST(ZONE, NAME, SIG, CACHE, LocOptions) FRONTEND_STATISTIC(Parse, NAME)
#include "polarphp/ast/ParseTypeIDZoneDef.h"
#undef POLAR_REQUEST

/// Number of conformances that were deserialized by this frontend job.
FRONTEND_STATISTIC(Sema, NumConformancesDeserialized)

/// Number of constraint-solving scopes created in the typechecker, while
/// solving expression type constraints. A rough proxy for "how much work the
/// expression typechecker did".
FRONTEND_STATISTIC(Sema, NumConstraintScopes)

/// Number of constraint-solving scopes that were created but which
/// did not themselves lead to the creation of further scopes, either
/// because we successfully found a solution, or some constraint failed.
///
/// Note: This can vary based on the number of connected components we
/// generate, since each connected component will itself have at least
/// one leaf scope.
FRONTEND_STATISTIC(Sema, NumLeafScopes)

/// Number of constraints considered per attempt to
/// contract constraint graph edges.
/// This is a measure of complexity of the contraction algorithm.
FRONTEND_STATISTIC(Sema, NumConstraintsConsideredForEdgeContraction)

/// Number of constraint-solving scopes created in the typechecker, while
/// solving expression type constraints. A rough proxy for "how much work the
/// expression typechecker did".
FRONTEND_STATISTIC(Sema, NumCyclicOneWayComponentsCollapsed)

/// Number of declarations that were deserialized. A rough proxy for the amount
/// of material loaded from other modules.
FRONTEND_STATISTIC(Sema, NumDeclsDeserialized)

/// Number of declarations type checked.
FRONTEND_STATISTIC(Sema, NumDeclsTypechecked)

/// Number of synthesized accessors.
FRONTEND_STATISTIC(Sema, NumAccessorsSynthesized)

/// Number of synthesized accessor bodies.
FRONTEND_STATISTIC(Sema, NumAccessorBodiesSynthesized)

/// Number of full function bodies typechecked.
FRONTEND_STATISTIC(Sema, NumFunctionsTypechecked)

/// Number of generic signature builders constructed. Rough proxy for
/// amount of work the GSB does analyzing type signatures.
FRONTEND_STATISTIC(Sema, NumGenericSignatureBuilders)

/// Number of lazy requirement signatures registered.
FRONTEND_STATISTIC(Sema, NumLazyRequirementSignatures)

/// Number of lazy requirement signatures deserialized.
FRONTEND_STATISTIC(Sema, NumLazyRequirementSignaturesLoaded)

/// Number of lazy iterable declaration contexts constructed.
FRONTEND_STATISTIC(Sema, NumLazyIterableDeclContexts)

/// Number of direct member-name lookups performed on nominal types.
FRONTEND_STATISTIC(Sema, NominalTypeLookupDirectCount)

/// Number of member-name lookups that avoided loading all members.
FRONTEND_STATISTIC(Sema, NamedLazyMemberLoadSuccessCount)

/// Number of member-name lookups that wound up loading all members.
FRONTEND_STATISTIC(Sema, NamedLazyMemberLoadFailureCount)

/// Number of types deserialized.
FRONTEND_STATISTIC(Sema, NumTypesDeserialized)

/// Number of types validated.
FRONTEND_STATISTIC(Sema, NumTypesValidated)

/// Number of lazy iterable declaration contexts left unloaded.
FRONTEND_STATISTIC(Sema, NumUnloadedLazyIterableDeclContexts)

/// Number of lookups into a module and its imports.

/// All type check requests go into the Sema area.
#define POLAR_REQUEST(ZONE, NAME, Sig, Caching, LocOptions) FRONTEND_STATISTIC(Sema, NAME)
#include "polarphp/ast/AccessTypeIDZoneDef.h"
#include "polarphp/ast/NameLookupTypeIDZoneDef.h"
#include "polarphp/ast/TypeCheckerTypeIDZoneDef.h"
#include "polarphp/sema/IDETypeCheckingRequestIDZoneDef.h"
#include "polarphp/ide/IDERequestIDZoneDef.h"
#undef POLAR_REQUEST

/// The next 10 statistics count 5 kinds of PIL entities present
/// after the PILGen and PILOpt phases. The entities are functions,
/// vtables, witness tables, default witness tables and global
/// variables.
FRONTEND_STATISTIC(PILModule, NumPILGenFunctions)
FRONTEND_STATISTIC(PILModule, NumPILGenVtables)
FRONTEND_STATISTIC(PILModule, NumPILGenWitnessTables)
FRONTEND_STATISTIC(PILModule, NumPILGenDefaultWitnessTables)
FRONTEND_STATISTIC(PILModule, NumPILGenGlobalVariables)

FRONTEND_STATISTIC(PILModule, NumPILOptFunctions)
FRONTEND_STATISTIC(PILModule, NumPILOptVtables)
FRONTEND_STATISTIC(PILModule, NumPILOptWitnessTables)
FRONTEND_STATISTIC(PILModule, NumPILOptDefaultWitnessTables)
FRONTEND_STATISTIC(PILModule, NumPILOptGlobalVariables)

/// The next 9 statistics count kinds of LLVM entities produced
/// during the IRGen phase: globals, functions, aliases, ifuncs,
/// named metadata, value and comdat symbols, basic blocks,
/// and instructions.
FRONTEND_STATISTIC(IRModule, NumIRGlobals)
FRONTEND_STATISTIC(IRModule, NumIRFunctions)
FRONTEND_STATISTIC(IRModule, NumIRAliases)
FRONTEND_STATISTIC(IRModule, NumIRIFuncs)
FRONTEND_STATISTIC(IRModule, NumIRNamedMetaData)
FRONTEND_STATISTIC(IRModule, NumIRValueSymbols)
FRONTEND_STATISTIC(IRModule, NumIRComdatSymbols)
FRONTEND_STATISTIC(IRModule, NumIRBasicBlocks)
FRONTEND_STATISTIC(IRModule, NumIRInsts)

/// Number of bytes written to the object-file output stream
/// of the frontend job, which should be the same as the size of
/// the .o file you find on disk after the frontend exits.
FRONTEND_STATISTIC(LLVM, NumLLVMBytesOutput)

#endif
