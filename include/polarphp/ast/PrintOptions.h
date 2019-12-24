//===--- PrintOptions.h - AST printing options ------------------*- C++ -*-===//
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

#ifndef POLARPHP_AST_PRINTOPTIONS_H
#define POLARPHP_AST_PRINTOPTIONS_H

#include "polarphp/basic/STLExtras.h"
#include "polarphp/ast/AttrKind.h"
#include "polarphp/ast/Identifier.h"
#include "polarphp/ast/TypeOrExtensionDecl.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/DenseMap.h"
#include <limits.h>
#include <vector>

namespace polar {
class AstPrinter;
class GenericEnvironment;
class CanType;
class Decl;
class Pattern;
class ValueDecl;
class ExtensionDecl;
class NominalTypeDecl;
class TypeBase;
class DeclContext;
class Type;
class ModuleDecl;
enum DeclAttrKind : unsigned;
class SynthesizedExtensionAnalyzer;
struct PrintOptions;

/// Necessary information for archetype transformation during printing.
struct TypeTransformContext {
   TypeBase *BaseType;
   TypeOrExtensionDecl Decl;

   explicit TypeTransformContext(Type T);
   explicit TypeTransformContext(TypeOrExtensionDecl D);

   Type getBaseType() const;
   TypeOrExtensionDecl getDecl() const;

   DeclContext *getDeclContext() const;

   bool isPrintingSynthesizedExtension() const;
};

class BracketOptions {
   Decl* Target;
   bool OpenExtension;
   bool CloseExtension;
   bool CloseNominal;

public:
   BracketOptions(Decl *Target = nullptr, bool OpenExtension = true,
                  bool CloseExtension = true, bool CloseNominal = true) :
      Target(Target), OpenExtension(OpenExtension),
      CloseExtension(CloseExtension),
      CloseNominal(CloseNominal) {}

   bool shouldOpenExtension(const Decl *D) {
      return D != Target || OpenExtension;
   }

   bool shouldCloseExtension(const Decl *D) {
      return D != Target || CloseExtension;
   }

   bool shouldCloseNominal(const Decl *D) {
      return D != Target || CloseNominal;
   }
};

/// A union of DeclAttrKind and TypeAttrKind.
class AnyAttrKind {
   unsigned kind : 31;
   unsigned isType : 1;

public:
   AnyAttrKind(TypeAttrKind K) : kind(static_cast<unsigned>(K)), isType(1) {
      static_assert(TAK_Count < UINT_MAX, "TypeAttrKind is > 31 bits");
   }
   AnyAttrKind(DeclAttrKind K) : kind(static_cast<unsigned>(K)), isType(0) {
      static_assert(DAK_Count < UINT_MAX, "DeclAttrKind is > 31 bits");
   }
   AnyAttrKind() : kind(TAK_Count), isType(1) {}
   AnyAttrKind(const AnyAttrKind &) = default;

   /// Returns the TypeAttrKind, or TAK_Count if this is not a type attribute.
   TypeAttrKind type() const {
      return isType ? static_cast<TypeAttrKind>(kind) : TAK_Count;
   }
   /// Returns the DeclAttrKind, or DAK_Count if this is not a decl attribute.
   DeclAttrKind decl() const {
      return isType ? DAK_Count : static_cast<DeclAttrKind>(kind);
   }

   bool operator==(AnyAttrKind K) const {
      return kind == K.kind && isType == K.isType;
   }
   bool operator!=(AnyAttrKind K) const { return !(*this == K); }
};

struct ShouldPrintChecker {
   virtual bool shouldPrint(const Decl *D, const PrintOptions &Options);
   bool shouldPrint(const Pattern *P, const PrintOptions &Options);
   virtual ~ShouldPrintChecker() = default;
};

/// Options for printing AST nodes.
///
/// A default-constructed PrintOptions is suitable for printing to users;
/// there are also factory methods for specific use cases.
struct PrintOptions {
   /// The indentation width.
   unsigned Indent = 2;

   /// Whether to print function definitions.
   bool FunctionDefinitions = false;

   /// Whether to print '{ get set }' on readwrite computed properties.
   bool PrintGetSetOnRWProperties = true;

   /// Whether to print *any* accessors on properties.
   bool PrintPropertyAccessors = true;

   /// Whether to print *any* accessors on subscript.
   bool PrintSubscriptAccessors = true;

   /// Whether to print the accessors of a property abstractly,
   /// i.e. always as:
   /// ```
   /// var x: Int { get set }
   /// ```
   /// rather than the specific accessors actually used to implement the
   /// property.
   ///
   /// Printing function definitions takes priority over this setting.
   bool AbstractAccessors = true;

   /// Whether to print a property with only a single getter using the shorthand
   /// ```
   /// var x: Int { return y }
   /// ```
   /// vs.
   /// ```
   /// var x: Int {
   ///   get { return y }
   /// }
   /// ```
   bool CollapseSingleGetterProperty = true;

   /// Whether to print the bodies of accessors in protocol context.
   bool PrintAccessorBodiesInInterfaces = false;

   /// Whether to print type definitions.
   bool TypeDefinitions = false;

   /// Whether to print variable initializers.
   bool VarInitializers = false;

   /// Choices for how to print enum raw values.
   enum class EnumRawValueMode {
      Skip,
      PrintObjCOnly,
      Print
   };

   /// Whether to print enum raw value expressions.
   EnumRawValueMode EnumRawValues = EnumRawValueMode::Skip;

   /// Whether to prefer printing TypeReprs instead of Types,
   /// if a TypeRepr is available.  This allows us to print the original
   /// spelling of the type name.
   ///
   /// \note This should be \c true when printing AST with the intention show
   /// it to the user.
   bool PreferTypeRepr = true;

   /// Whether to print fully qualified Types.
   bool FullyQualifiedTypes = false;

   /// Print fully qualified types if our heuristics say that a certain
   /// type might be ambiguous.
   bool FullyQualifiedTypesIfAmbiguous = false;

   /// If true, printed module names will use the "exported" name, which may be
   /// different from the regular name.
   ///
   /// \see FileUnit::getExportedModuleName
   bool UseExportedModuleNames = false;

   /// Print Swift.Array and Swift.Optional with sugared syntax
   /// ([] and ?), even if there are no sugar type nodes.
   bool SynthesizeSugarOnTypes = false;

   /// If true, null types in the AST will be printed as "<null>". If
   /// false, the compiler will trap.
   bool AllowNullTypes = true;

   /// If true, the printer will explode a pattern like this:
   /// \code
   ///   var (a, b) = f()
   /// \endcode
   /// into multiple variable declarations.
   ///
   /// For this option to work correctly, \c VarInitializers should be
   /// \c false.
   bool ExplodePatternBindingDecls = false;

   /// If true, the printer will explode an enum case like this:
   /// \code
   ///   case A, B
   /// \endcode
   /// into multiple case declarations.
   bool ExplodeEnumCaseDecls = false;

   /// Whether to print implicit parts of the AST.
   bool SkipImplicit = false;

   /// Whether to print unavailable parts of the AST.
   bool SkipUnavailable = false;

   /// Whether to skip internal stdlib declarations.
   bool SkipPrivateStdlibDecls = false;

   /// Whether to skip underscored stdlib protocols.
   /// Protocols marked with @_show_in_interface are still printed.
   bool SkipUnderscoredStdlibInterfaces = false;

   /// Whether to skip extensions that don't add protocols or no members.
   bool SkipEmptyExtensionDecls = true;

   /// Whether to print attributes.
   bool SkipAttributes = false;

   /// Whether to print keywords like 'func'.
   bool SkipIntroducerKeywords = false;

   /// Whether to print destructors.
   bool SkipDeinit = false;

   /// Whether to skip printing 'import' declarations.
   bool SkipImports = false;

   /// Whether to skip printing overrides and witnesses for
   /// protocol requirements.
   bool SkipOverrides = false;

   /// Whether to skip placeholder members.
   bool SkipMissingMemberPlaceholders = true;

   /// Whether to print a long attribute like '\@available' on a separate line
   /// from the declaration or other attributes.
   bool PrintLongAttrsOnSeparateLines = false;

   bool PrintImplicitAttrs = true;

   /// Whether to skip keywords with a prefix of underscore such as __consuming.
   bool SkipUnderscoredKeywords = false;

   /// Prints type variables and unresolved types in an expanded notation suitable
   /// for debugging.
   bool PrintTypesForDebugging = false;

   /// How to print opaque return types.
   enum class OpaqueReturnTypePrintingMode {
      /// 'some P1 & P2'.
         WithOpaqueKeyword,
      /// 'P1 & P2'.
         WithoutOpaqueKeyword,
      /// Stable parsable internal syntax.
         StableReference,
      /// Description suitable for debugging.
         Description
   };

   OpaqueReturnTypePrintingMode OpaqueReturnTypePrinting =
      OpaqueReturnTypePrintingMode::WithOpaqueKeyword;

   /// Whether to print decl attributes that are only used internally,
   /// such as _silgen_name, transparent, etc.
   bool PrintUserInaccessibleAttrs = true;

   /// List of attribute kinds that should not be printed.
   std::vector<AnyAttrKind> ExcludeAttrList = {DAK_Transparent, DAK_Effects,
                                               DAK_FixedLayout,
                                               DAK_ShowInInterface};

   /// List of attribute kinds that should be printed exclusively.
   /// Empty means allow all.
   std::vector<AnyAttrKind> ExclusiveAttrList;

   /// List of decls that should be printed even if they are implicit and \c SkipImplicit is set to true.
   std::vector<const Decl*> TreatAsExplicitDeclList;

   /// Whether to print function @convention attribute on function types.
   bool PrintFunctionRepresentationAttrs = true;

   /// Whether to print storage representation attributes on types, e.g.
   /// '@sil_weak', '@sil_unmanaged'.
   bool PrintStorageRepresentationAttrs = false;

   /// Whether to print 'static' or 'class' on static decls.
   bool PrintStaticKeyword = true;

   /// Whether to print 'override' keyword on overridden decls.
   bool PrintOverrideKeyword = true;

   /// Whether to print access control information on all value decls.
   bool PrintAccess = false;

   /// If \c PrintAccess is true, this determines whether to print
   /// 'internal' keyword.
   bool PrintInternalAccessKeyword = true;

   /// Print all decls that have at least this level of access.
   AccessLevel AccessFilter = AccessLevel::Private;

   /// Print IfConfigDecls.
   bool PrintIfConfig = true;

   /// Whether we are printing for sil.
   bool PrintForPIL = false;

   /// Whether we are printing part of PIL body.
   bool PrintInPILBody = false;

   /// Whether to use an empty line to separate two members in a single decl.
   bool EmptyLineBetweenMembers = false;

   /// Whether to print the extensions from conforming protocols.
   bool PrintExtensionFromConformingInterfaces = false;

   std::shared_ptr<ShouldPrintChecker> CurrentPrintabilityChecker =
      std::make_shared<ShouldPrintChecker>();

   enum class ArgAndParamPrintingMode {
      ArgumentOnly,
      MatchSource,
      BothAlways,
      EnumElement,
   };

   /// Whether to print the doc-comment from the conformance if a member decl
   /// has no associated doc-comment by itself.
   bool CascadeDocComment = false;

   /// Whether to print the content of an extension decl inside the type decl where it
   /// extends from.
   std::function<bool(const ExtensionDecl *)> printExtensionContentAsMembers =
      [] (const ExtensionDecl *) { return false; };

   /// How to print the keyword argument and parameter name in functions.
   ArgAndParamPrintingMode ArgAndParamPrinting =
      ArgAndParamPrintingMode::MatchSource;

   /// Whether to print documentation comments attached to declarations.
   /// Note that this may print documentation comments from related declarations
   /// (e.g. the overridden method in the superclass) if such comment is found.
   bool PrintDocumentationComments = false;

   /// Whether to print regular comments from clang module headers.
   bool PrintRegularClangComments = false;

   /// When true, printing interface from a source file will print the original
   /// source text for applicable declarations, in order to preserve the
   /// formatting.
   bool PrintOriginalSourceText = false;

   /// When printing a type alias type, whether print the underlying type instead
   /// of the alias.
   bool PrintTypeAliasUnderlyingType = false;

   /// When printing an Optional<T>, rather than printing 'T?', print
   /// 'T!'. Used as a modifier only when we know we're printing
   /// something that was declared as an implicitly unwrapped optional
   /// at the top level. This is stripped out of the printing options
   /// for optionals that are nested within other optionals.
   bool PrintOptionalAsImplicitlyUnwrapped = false;

   /// Replaces the name of private and internal properties of types with '_'.
   bool OmitNameOfInaccessibleProperties = false;

   /// Print dependent types as references into this generic environment.
   GenericEnvironment *GenericEnv = nullptr;

   /// Print types with alternative names from their canonical names.
   llvm::DenseMap<CanType, Identifier> *AlternativeTypeNames = nullptr;

   /// The module in which the printer is used. Determines if the module
   /// name should be printed when printing a type.
   ModuleDecl *CurrentModule = nullptr;

   /// The information for converting archetypes to specialized types.
   llvm::Optional<TypeTransformContext> TransformContext;

   bool PrintAsMember = false;

   /// Whether to print parameter specifiers as 'let' and 'var'.
   bool PrintParameterSpecifiers = false;

   /// \see ShouldQualifyNestedDeclarations
   enum class QualifyNestedDeclarations {
      Never,
      TypesOnly,
      Always
   };

   /// Controls when a nested declaration's name should be printed qualified with
   /// its enclosing context, if it's being printed on its own (rather than as
   /// part of the context).
   QualifyNestedDeclarations ShouldQualifyNestedDeclarations =
      QualifyNestedDeclarations::Never;

   /// If this is not \c nullptr then function bodies (including accessors
   /// and constructors) will be printed by this function.
   std::function<void(const ValueDecl *, AstPrinter &)> FunctionBody;

   BracketOptions bracketOptions;

   // This is explicit to guarantee that it can be called from LLDB.
   PrintOptions() {}

   bool excludeAttrKind(AnyAttrKind K) const {
      if (std::any_of(ExcludeAttrList.begin(), ExcludeAttrList.end(),
                      [K](AnyAttrKind other) { return other == K; }))
         return true;
      if (!ExclusiveAttrList.empty())
         return std::none_of(ExclusiveAttrList.begin(), ExclusiveAttrList.end(),
                             [K](AnyAttrKind other) { return other == K; });
      return false;
   }

   /// Retrieve the set of options for verbose printing to users.
   static PrintOptions printVerbose() {
      PrintOptions result;
      result.TypeDefinitions = true;
      result.VarInitializers = true;
      result.PrintDocumentationComments = true;
      result.PrintRegularClangComments = true;
      result.PrintLongAttrsOnSeparateLines = true;
      return result;
   }

   /// Retrieve the set of options suitable for diagnostics printing.
   static PrintOptions printForDiagnostics() {
      PrintOptions result = printVerbose();
      result.PrintAccess = true;
      result.Indent = 4;
      result.FullyQualifiedTypesIfAmbiguous = true;
      result.SynthesizeSugarOnTypes = true;
      result.PrintUserInaccessibleAttrs = false;
      result.PrintImplicitAttrs = false;
      result.ExcludeAttrList.push_back(DAK_Exported);
      result.ExcludeAttrList.push_back(DAK_Inline);
      result.ExcludeAttrList.push_back(DAK_Optimize);
      result.ExcludeAttrList.push_back(DAK_Rethrows);
      result.PrintOverrideKeyword = false;
      result.AccessFilter = AccessLevel::Public;
      result.PrintIfConfig = false;
      result.ShouldQualifyNestedDeclarations =
         QualifyNestedDeclarations::TypesOnly;
      result.PrintDocumentationComments = false;
      return result;
   }

   /// Retrieve the set of options suitable for interface generation.
   static PrintOptions printInterface() {
      PrintOptions result = printForDiagnostics();
      result.SkipUnavailable = true;
      result.SkipImplicit = true;
      result.SkipPrivateStdlibDecls = true;
      result.SkipUnderscoredStdlibInterfaces = true;
      result.SkipDeinit = true;
      result.ExcludeAttrList.push_back(DAK_DiscardableResult);
      result.EmptyLineBetweenMembers = true;
      result.CascadeDocComment = true;
      result.ShouldQualifyNestedDeclarations =
         QualifyNestedDeclarations::Always;
      result.PrintDocumentationComments = true;
      result.SkipUnderscoredKeywords = true;
      result.EnumRawValues = EnumRawValueMode::PrintObjCOnly;
      return result;
   }

   /// Retrieve the set of options suitable for module interfaces.
   ///
   /// This is a format that will be parsed again later, so the output must be
   /// consistent and well-formed.
   ///
   /// \see swift::emitSwiftInterface
   static PrintOptions printSwiftInterfaceFile(bool preferTypeRepr);

   /// Retrieve the set of options suitable for "Generated Interfaces", which
   /// are a prettified representation of the public API of a module, to be
   /// displayed to users in an editor.
   static PrintOptions printModuleInterface();
   static PrintOptions printTypeInterface(Type T);

   void setBaseType(Type T);

   void initForSynthesizedExtension(TypeOrExtensionDecl D);

   void clearSynthesizedExtension();

   bool shouldPrint(const Decl* D) const {
      return CurrentPrintabilityChecker->shouldPrint(D, *this);
   }
   bool shouldPrint(const Pattern* P) const {
      return CurrentPrintabilityChecker->shouldPrint(P, *this);
   }

   /// Retrieve the print options that are suitable to print the testable interface.
   static PrintOptions printTestableInterface() {
      PrintOptions result = printInterface();
      result.AccessFilter = AccessLevel::Interface;
      return result;
   }

   /// Retrieve the print options that are suitable to print interface for a
   /// swift file.
   static PrintOptions printSwiftFileInterface() {
      PrintOptions result = printInterface();
      result.AccessFilter = AccessLevel::Interface;
      result.EmptyLineBetweenMembers = true;
      return result;
   }

   /// Retrieve the set of options suitable for interface generation for
   /// documentation purposes.
   static PrintOptions printDocInterface();

   /// Retrieve the set of options suitable for printing PIL functions.
   static PrintOptions printPIL() {
      PrintOptions result;
      result.PrintLongAttrsOnSeparateLines = true;
      result.PrintStorageRepresentationAttrs = true;
      result.AbstractAccessors = false;
      result.PrintForPIL = true;
      result.PrintInPILBody = true;
      result.PreferTypeRepr = false;
      result.PrintIfConfig = false;
      result.OpaqueReturnTypePrinting =
         OpaqueReturnTypePrintingMode::StableReference;
      return result;
   }

   static PrintOptions printQualifiedPILType() {
      PrintOptions result = PrintOptions::printPIL();
      result.FullyQualifiedTypesIfAmbiguous = true;
      return result;
   }

   /// Retrieve the set of options that prints everything.
   ///
   /// This is only intended for debug output.
   static PrintOptions printEverything() {
      PrintOptions result = printVerbose();
      result.ExcludeAttrList.clear();
      result.ExcludeAttrList.push_back(DAK_FixedLayout);
      result.PrintStorageRepresentationAttrs = true;
      result.AbstractAccessors = false;
      result.PrintAccess = true;
      result.SkipEmptyExtensionDecls = false;
      result.SkipMissingMemberPlaceholders = false;
      return result;
   }

   /// Print in the style of quick help declaration.
   static PrintOptions printQuickHelpDeclaration() {
      PrintOptions PO;
      PO.SkipUnderscoredKeywords = true;
      PO.EnumRawValues = EnumRawValueMode::Print;
      PO.PrintImplicitAttrs = false;
      PO.PrintFunctionRepresentationAttrs = false;
      PO.PrintDocumentationComments = false;
      PO.ExcludeAttrList.push_back(DAK_Available);
      PO.SkipPrivateStdlibDecls = true;
      PO.ExplodeEnumCaseDecls = true;
      PO.ShouldQualifyNestedDeclarations = QualifyNestedDeclarations::TypesOnly;
      PO.PrintParameterSpecifiers = true;
      PO.SkipImplicit = true;
      return PO;
   }
};
} // polar

#endif // POLARPHP_AST_PRINTOPTIONS_H
