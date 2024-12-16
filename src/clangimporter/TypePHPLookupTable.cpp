//===--- TypePHPLookupTable.cpp - Swift Lookup Table ------------------------===//
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
// This file implements support for Swift name lookup tables stored in Clang
// modules.
//
//===----------------------------------------------------------------------===//
#include "polarphp/clangimporter/internal/ImporterImpl.h"
#include "polarphp/clangimporter/internal/TypePHPLookupTable.h"
#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/ast/DiagnosticsClangImporter.h"
#include "polarphp/basic/StlExtras.h"
#include "polarphp/basic/Version.h"
#include "clang/AST/DeclObjC.h"
#include "clang/Lex/MacroInfo.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Sema/Sema.h"
#include "clang/Serialization/ASTBitCodes.h"
#include "clang/Serialization/ASTReader.h"
#include "clang/Serialization/ASTWriter.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Bitstream/BitstreamReader.h"
#include "llvm/Bitstream/BitstreamWriter.h"
#include "llvm/Support/DJB.h"
#include "llvm/Support/OnDiskHashTable.h"

using namespace polar;
using namespace importer;
using namespace llvm::support;

/// Determine whether the new declarations matches an existing declaration.
static bool matchesExistingDecl(clang::Decl *decl, clang::Decl *existingDecl) {
   // If the canonical declarations are equivalent, we have a match.
   if (decl->getCanonicalDecl() == existingDecl->getCanonicalDecl()) {
      return true;
   }

   return false;
}

namespace {
class BaseNameToEntitiesTableReaderInfo;
class GlobalsAsMembersTableReaderInfo;

using SerializedBaseNameToEntitiesTable =
llvm::OnDiskIterableChainedHashTable<BaseNameToEntitiesTableReaderInfo>;

using SerializedGlobalsAsMembersTable =
llvm::OnDiskIterableChainedHashTable<GlobalsAsMembersTableReaderInfo>;
} // end anonymous namespace

namespace polar {
/// Module file extension writer for the Swift lookup tables.
class TypePHPLookupTableWriter : public clang::ModuleFileExtensionWriter {
   clang::ASTWriter &Writer;

   AstContext &typePHPCtx;
   importer::ClangSourceBufferImporter &buffersForDiagnostics;
   const PlatformAvailability &availability;
   const bool inferImportAsMember;

public:
   TypePHPLookupTableWriter(
      clang::ModuleFileExtension *extension, clang::ASTWriter &writer,
      AstContext &ctx,
      importer::ClangSourceBufferImporter &buffersForDiagnostics,
      const PlatformAvailability &avail, bool inferIAM)
      : ModuleFileExtensionWriter(extension), Writer(writer), typePHPCtx(ctx),
        buffersForDiagnostics(buffersForDiagnostics), availability(avail),
        inferImportAsMember(inferIAM) {}

   void writeExtensionContents(clang::Sema &sema,
                               llvm::BitstreamWriter &stream) override;

   void populateTable(TypePHPLookupTable &table, NameImporter &);

   void populateTableWithDecl(TypePHPLookupTable &table,
                              NameImporter &nameImporter, clang::Decl *decl);
};

/// Module file extension reader for the Swift lookup tables.
class TypePHPLookupTableReader : public clang::ModuleFileExtensionReader {
   clang::ASTReader &Reader;
   clang::serialization::ModuleFile &ModuleFile;
   std::function<void()> OnRemove;

   std::unique_ptr<SerializedBaseNameToEntitiesTable> SerializedTable;
   ArrayRef<clang::serialization::DeclID> Categories;
   std::unique_ptr<SerializedGlobalsAsMembersTable> GlobalsAsMembersTable;

   TypePHPLookupTableReader(clang::ModuleFileExtension *extension,
                            clang::ASTReader &reader,
                            clang::serialization::ModuleFile &moduleFile,
                            std::function<void()> onRemove,
                            std::unique_ptr<SerializedBaseNameToEntitiesTable>
                            serializedTable,
                            ArrayRef<clang::serialization::DeclID> categories,
                            std::unique_ptr<SerializedGlobalsAsMembersTable>
                            globalsAsMembersTable)
      : ModuleFileExtensionReader(extension), Reader(reader),
        ModuleFile(moduleFile), OnRemove(onRemove),
        SerializedTable(std::move(serializedTable)), Categories(categories),
        GlobalsAsMembersTable(std::move(globalsAsMembersTable)) {}

public:
   /// Create a new lookup table reader for the given AST reader and stream
   /// position.
   static std::unique_ptr<TypePHPLookupTableReader>
   create(clang::ModuleFileExtension *extension, clang::ASTReader &reader,
          clang::serialization::ModuleFile &moduleFile,
          std::function<void()> onRemove, const llvm::BitstreamCursor &stream);

   ~TypePHPLookupTableReader() override;

   /// Retrieve the AST reader associated with this lookup table reader.
   clang::ASTReader &getASTReader() const { return Reader; }

   /// Retrieve the module file associated with this lookup table reader.
   clang::serialization::ModuleFile &getModuleFile() { return ModuleFile; }

   /// Retrieve the set of base names that are stored in the on-disk hash table.
   SmallVector<SerializedTypePHPName, 4> getBaseNames();

   /// Retrieve the set of entries associated with the given base name.
   ///
   /// \returns true if we found anything, false otherwise.
   bool lookup(SerializedTypePHPName baseName,
               SmallVectorImpl<TypePHPLookupTable::FullTableEntry> &entries);

   /// Retrieve the declaration IDs of the categories.
   ArrayRef<clang::serialization::DeclID> categories() const {
      return Categories;
   }

   /// Retrieve the set of contexts that have globals-as-members
   /// injected into them.
   SmallVector<TypePHPLookupTable::StoredContext, 4> getGlobalsAsMembersContexts();

   /// Retrieve the set of global declarations that are going to be
   /// imported as members into the given context.
   ///
   /// \returns true if we found anything, false otherwise.
   bool lookupGlobalsAsMembers(TypePHPLookupTable::StoredContext context,
                               SmallVectorImpl<uint64_t> &entries);
};
} // namespace polarphp

DeclBaseName SerializedTypePHPName::toDeclBaseName(AstContext &Context) const {
   switch (Kind) {
      case DeclBaseName::Kind::Normal:
         return Context.getIdentifier(Name);
      case DeclBaseName::Kind::Subscript:
         return DeclBaseName::createSubscript();
      case DeclBaseName::Kind::Constructor:
         return DeclBaseName::createConstructor();
      case DeclBaseName::Kind::Destructor:
         return DeclBaseName::createDestructor();
   }
   llvm_unreachable("unhandled kind");
}

bool TypePHPLookupTable::contextRequiresName(ContextKind kind) {
   switch (kind) {
      case ContextKind::ObjCClass:
      case ContextKind::ObjCProtocol:
      case ContextKind::Tag:
      case ContextKind::Typedef:
         return true;

      case ContextKind::TranslationUnit:
         return false;
   }

   llvm_unreachable("Invalid ContextKind.");
}

/// Try to translate the given Clang declaration into a context.
static Optional<TypePHPLookupTable::StoredContext>
translateDeclToContext(clang::NamedDecl *decl) {
   // Tag declaration.
   if (auto tag = dyn_cast<clang::TagDecl>(decl)) {
      if (tag->getIdentifier())
         return std::make_pair(TypePHPLookupTable::ContextKind::Tag, tag->getName());
      if (auto typedefDecl = tag->getTypedefNameForAnonDecl())
         return std::make_pair(TypePHPLookupTable::ContextKind::Tag,
                               typedefDecl->getName());
      return None;
   }

   // Namespace declaration.
   if (auto namespaceDecl = dyn_cast<clang::NamespaceDecl>(decl)) {
      if (namespaceDecl->getIdentifier())
         return std::make_pair(TypePHPLookupTable::ContextKind::Tag,
                               namespaceDecl->getName());
      return None;
   }

   // Objective-C class context.
   if (auto objcClass = dyn_cast<clang::ObjCInterfaceDecl>(decl))
      return std::make_pair(TypePHPLookupTable::ContextKind::ObjCClass,
                            objcClass->getName());

   // Objective-C protocol context.
   if (auto objcProtocol = dyn_cast<clang::ObjCProtocolDecl>(decl))
      return std::make_pair(TypePHPLookupTable::ContextKind::ObjCProtocol,
                            objcProtocol->getName());

   // Typedefs.
   if (auto typedefName = dyn_cast<clang::TypedefNameDecl>(decl)) {
      // If this typedef is merely a restatement of a tag declaration's type,
      // return the result for that tag.
      if (auto tag = typedefName->getUnderlyingType()->getAsTagDecl())
         return translateDeclToContext(const_cast<clang::TagDecl *>(tag));

      // Otherwise, this must be a typedef mapped to a strong type.
      return std::make_pair(TypePHPLookupTable::ContextKind::Typedef,
                            typedefName->getName());
   }

   return None;
}

auto TypePHPLookupTable::translateDeclContext(const clang::DeclContext *dc)
-> Optional<TypePHPLookupTable::StoredContext> {
   // Translation unit context.
   if (dc->isTranslationUnit())
      return std::make_pair(ContextKind::TranslationUnit, StringRef());

   // Tag declaration context.
   if (auto tag = dyn_cast<clang::TagDecl>(dc))
      return translateDeclToContext(const_cast<clang::TagDecl *>(tag));

   // Namespace declaration context.
   if (auto namespaceDecl = dyn_cast<clang::NamespaceDecl>(dc))
      return translateDeclToContext(
         const_cast<clang::NamespaceDecl *>(namespaceDecl));

   // Objective-C class context.
   if (auto objcClass = dyn_cast<clang::ObjCInterfaceDecl>(dc))
      return std::make_pair(ContextKind::ObjCClass, objcClass->getName());

   // Objective-C protocol context.
   if (auto objcProtocol = dyn_cast<clang::ObjCProtocolDecl>(dc))
      return std::make_pair(ContextKind::ObjCProtocol, objcProtocol->getName());

   return None;
}

Optional<TypePHPLookupTable::StoredContext>
TypePHPLookupTable::translateContext(EffectiveClangContext context) {
   switch (context.getKind()) {
      case EffectiveClangContext::DeclContext: {
         return translateDeclContext(context.getAsDeclContext());
      }

      case EffectiveClangContext::TypedefContext:
         return std::make_pair(ContextKind::Typedef,
                               context.getTypedefName()->getName());

      case EffectiveClangContext::UnresolvedContext:
         // Resolve the context.
         if (auto decl = resolveContext(context.getUnresolvedName()))
            return translateDeclToContext(decl);

         return None;
   }

   llvm_unreachable("Invalid EffectiveClangContext.");
}

/// Lookup an unresolved context name and resolve it to a Clang
/// declaration context or typedef name.
clang::NamedDecl *TypePHPLookupTable::resolveContext(StringRef unresolvedName) {
   // Look for a context with the given Swift name.
   for (auto entry :
      lookup(SerializedTypePHPName(unresolvedName),
             std::make_pair(ContextKind::TranslationUnit, StringRef()))) {
      if (auto decl = entry.dyn_cast<clang::NamedDecl *>()) {
         if (isa<clang::TagDecl>(decl) ||
             isa<clang::ObjCInterfaceDecl>(decl) ||
             isa<clang::TypedefNameDecl>(decl))
            return decl;
      }
   }

   // FIXME: Search imported modules to resolve the context.

   return nullptr;
}

void TypePHPLookupTable::addCategory(clang::ObjCCategoryDecl *category) {
   // Force deserialization to occur before appending.
   (void) categories();

   // Add the category.
   Categories.push_back(category);
}

bool TypePHPLookupTable::resolveUnresolvedEntries(
   SmallVectorImpl<SingleEntry> &unresolved) {
   // Common case: nothing left to resolve.
   unresolved.clear();
   if (UnresolvedEntries.empty()) return false;

   // Reprocess each of the unresolved entries to see if it can be
   // resolved now that we're done. This occurs when a swift_name'd
   // entity becomes a member of an entity that follows it in the
   // translation unit, e.g., given:
   //
   // \code
   //   typedef enum FooSomeEnumeration __attribute__((Foo.SomeEnum)) {
   //     ...
   //   } FooSomeEnumeration;
   //
   //   typedef struct Foo {
   //
   //   } Foo;
   // \endcode
   //
   // FooSomeEnumeration belongs inside "Foo", but we haven't actually
   // seen "Foo" yet. Therefore, we will reprocess FooSomeEnumeration
   // at the end, once "Foo" is available. There are several reasons
   // this loop can execute:
   //
   // * Import-as-member places an entity inside of an another entity
   // that comes later in the translation unit. The number of
   // iterations that can be caused by this is bounded by the nesting
   // depth. (At present, that depth is limited to 2).
   //
   // * An erroneous import-as-member will cause an extra iteration at
   // the end, so that the loop can detect that nothing changed and
   // return a failure.
   while (true) {
      // Take the list of unresolved entries to process.
      auto prevNumUnresolvedEntries = UnresolvedEntries.size();
      auto currentUnresolved = std::move(UnresolvedEntries);
      UnresolvedEntries.clear();

      // Process each of the currently-unresolved entries.
      for (const auto &entry : currentUnresolved)
         addEntry(std::get<0>(entry), std::get<1>(entry), std::get<2>(entry));

      // Are we done?
      if (UnresolvedEntries.empty()) return false;

      // If nothing changed, fail: something is unresolvable, and the
      // caller should complain.
      if (UnresolvedEntries.size() == prevNumUnresolvedEntries) {
         for (const auto &entry : UnresolvedEntries)
            unresolved.push_back(std::get<1>(entry));
         return true;
      }

      // Something got resolved, so loop again.
      assert(UnresolvedEntries.size() < prevNumUnresolvedEntries);
   }
}

/// Determine whether the entry is a global declaration that is being
/// mapped as a member of a particular type or extension thereof.
///
/// This should only return true when the entry isn't already nested
/// within a context. For example, it will return false for
/// enumerators, because those are naturally nested within the
/// enumeration declaration.
static bool isGlobalAsMember(TypePHPLookupTable::SingleEntry entry,
                             TypePHPLookupTable::StoredContext context) {
   switch (context.first) {
      case TypePHPLookupTable::ContextKind::TranslationUnit:
         // We're not mapping this as a member of anything.
         return false;

      case TypePHPLookupTable::ContextKind::Tag:
      case TypePHPLookupTable::ContextKind::ObjCClass:
      case TypePHPLookupTable::ContextKind::ObjCProtocol:
      case TypePHPLookupTable::ContextKind::Typedef:
         // We're mapping into a type context.
         break;
   }

   // Macros are never stored within a non-translation-unit context in
   // Clang.
   if (entry.is<clang::MacroInfo *>()) return true;

   // We have a declaration.
   auto decl = entry.get<clang::NamedDecl *>();

   // Enumerators have the translation unit as their redeclaration context,
   // but members of anonymous enums are still allowed to be in the
   // global-as-member category.
   if (isa<clang::EnumConstantDecl>(decl)) {
      const auto *theEnum = cast<clang::EnumDecl>(decl->getDeclContext());
      return !theEnum->hasNameForLinkage();
   }

   // If the redeclaration context is namespace-scope, then we're
   // mapping as a member.
   return decl->getDeclContext()->getRedeclContext()->isFileContext();
}

bool TypePHPLookupTable::addLocalEntry(SingleEntry newEntry,
                                       SmallVectorImpl<uint64_t> &entries) {
   // Check whether this entry matches any existing entry.
   auto decl = newEntry.dyn_cast<clang::NamedDecl *>();
   auto macro = newEntry.dyn_cast<clang::MacroInfo *>();
   auto moduleMacro = newEntry.dyn_cast<clang::ModuleMacro *>();

   for (auto &existingEntry : entries) {
      // If it matches an existing declaration, there's nothing to do.
      if (decl && isDeclEntry(existingEntry) &&
          matchesExistingDecl(decl, mapStoredDecl(existingEntry)))
         return false;

      // If a textual macro matches an existing macro, just drop the new
      // definition.
      if (macro && isMacroEntry(existingEntry)) {
         return false;
      }

      // If a module macro matches an existing macro, be a bit more discerning.
      //
      // Specifically, if the innermost explicit submodule containing the new
      // macro contains the innermost explicit submodule containing the existing
      // macro, the new one should replace the old one; if they're the same
      // module, the old one should stay in place. Otherwise, they don't share an
      // explicit module, and should be considered alternatives.
      //
      // Note that the above assumes that macro definitions are processed in
      // reverse order, i.e. the first definition seen is the last in a
      // translation unit.
      if (moduleMacro && isMacroEntry(existingEntry)) {
         SingleEntry decodedEntry = mapStoredMacro(existingEntry,
            /*assumeModule*/true);
         const auto *existingMacro = decodedEntry.get<clang::ModuleMacro *>();

         const clang::Module *newModule = moduleMacro->getOwningModule();
         const clang::Module *existingModule = existingMacro->getOwningModule();

         // A simple redeclaration: drop the new definition.
         if (existingModule == newModule)
            return false;

         // A broader-scoped redeclaration: drop the old definition.
         if (existingModule->isSubModuleOf(newModule)) {
            // FIXME: What if there are /multiple/ old definitions we should be
            // dropping? What if one of the earlier early exits makes us miss
            // entries later in the list that would match this?
            existingEntry = encodeEntry(moduleMacro);
            return false;
         }

         // Otherwise, just allow both definitions to coexist.
      }
   }

   // Add an entry to this context.
   if (decl)
      entries.push_back(encodeEntry(decl));
   else if (macro)
      entries.push_back(encodeEntry(macro));
   else
      entries.push_back(encodeEntry(moduleMacro));
   return true;
}

void TypePHPLookupTable::addEntry(DeclName name, SingleEntry newEntry,
                                  EffectiveClangContext effectiveContext) {
   assert(newEntry);

   // Translate the context.
   auto contextOpt = translateContext(effectiveContext);
   if (!contextOpt) {
      // We might be able to resolve this later.
      if (newEntry.is<clang::NamedDecl *>()) {
         UnresolvedEntries.push_back(
            std::make_tuple(name, newEntry, effectiveContext));
      }

      return;
   }

   // Populate cache from reader if necessary.
   findOrCreate(name.getBaseName());

   auto context = *contextOpt;

   // If this is a global imported as a member, record is as such.
   if (isGlobalAsMember(newEntry, context)) {
      auto &entries = GlobalsAsMembers[context];
      (void)addLocalEntry(newEntry, entries);
   }

   // Find the list of entries for this base name.
   auto &entries = LookupTable[name.getBaseName()];
   auto decl = newEntry.dyn_cast<clang::NamedDecl *>();
   auto macro = newEntry.dyn_cast<clang::MacroInfo *>();
   auto moduleMacro = newEntry.dyn_cast<clang::ModuleMacro *>();
   for (auto &entry : entries) {
      if (entry.Context == context) {
         // We have entries for this context.
         (void)addLocalEntry(newEntry, entry.DeclsOrMacros);
         return;
      }
   }

   // This is a new context for this name. Add it.
   FullTableEntry entry;
   entry.Context = context;
   if (decl)
      entry.DeclsOrMacros.push_back(encodeEntry(decl));
   else if (macro)
      entry.DeclsOrMacros.push_back(encodeEntry(macro));
   else
      entry.DeclsOrMacros.push_back(encodeEntry(moduleMacro));
   entries.push_back(entry);
}

auto TypePHPLookupTable::findOrCreate(SerializedTypePHPName baseName)
-> llvm::DenseMap<SerializedTypePHPName,
   SmallVector<FullTableEntry, 2>>::iterator {
// If there is no base name, there is nothing to find.
   if (baseName.empty()) return LookupTable.end();

// Find entries for this base name.
   auto known = LookupTable.find(baseName);

// If we found something, we're done.
   if (known != LookupTable.end()) return known;

// If there's no reader, we've found all there is to find.
   if (!Reader) return known;

// Lookup this base name in the module file.
   SmallVector<FullTableEntry, 2> results;
   (void)Reader->lookup(baseName, results);

// Add an entry to the table so we don't look again.
   known = LookupTable.insert({ std::move(baseName), std::move(results) }).first;

   return known;
}

SmallVector<TypePHPLookupTable::SingleEntry, 4>
TypePHPLookupTable::lookup(SerializedTypePHPName baseName,
                           llvm::Optional<StoredContext> searchContext) {
   SmallVector<TypePHPLookupTable::SingleEntry, 4> result;

   // Find the lookup table entry for this base name.
   auto known = findOrCreate(baseName);
   if (known == LookupTable.end()) return result;

   // Walk each of the entries.
   for (auto &entry : known->second) {
      // If we're looking in a particular context and it doesn't match the
      // entry context, we're done.
      if (searchContext && entry.Context != *searchContext)
         continue;

      // Map each of the declarations.
      for (auto &stored : entry.DeclsOrMacros)
         if (auto entry = mapStored(stored))
            result.push_back(entry);
   }

   return result;
}

SmallVector<TypePHPLookupTable::SingleEntry, 4>
TypePHPLookupTable::lookupGlobalsAsMembers(StoredContext context) {
   SmallVector<TypePHPLookupTable::SingleEntry, 4> result;

   // Find entries for this base name.
   auto known = GlobalsAsMembers.find(context);

   // If we didn't find anything...
   if (known == GlobalsAsMembers.end()) {
      // If there's no reader, we've found all there is to find.
      if (!Reader) return result;

      // Lookup this base name in the module extension file.
      SmallVector<uint64_t, 2> results;
      (void)Reader->lookupGlobalsAsMembers(context, results);

      // Add an entry to the table so we don't look again.
      known = GlobalsAsMembers.insert({ std::move(context),
                                        std::move(results) }).first;
   }

   // Map each of the results.
   for (auto &entry : known->second) {
      result.push_back(mapStored(entry));
   }

   return result;
}

SmallVector<TypePHPLookupTable::SingleEntry, 4>
TypePHPLookupTable::lookupGlobalsAsMembers(EffectiveClangContext context) {
   // Translate context.
   if (!context) return { };

   Optional<StoredContext> storedContext = translateContext(context);
   if (!storedContext) return { };

   return lookupGlobalsAsMembers(*storedContext);
}

SmallVector<TypePHPLookupTable::SingleEntry, 4>
TypePHPLookupTable::allGlobalsAsMembers() {
   // If we have a reader, deserialize all of the globals-as-members data.
   if (Reader) {
      for (auto context : Reader->getGlobalsAsMembersContexts()) {
         (void)lookupGlobalsAsMembers(context);
      }
   }

   // Collect all of the keys and sort them.
   SmallVector<StoredContext, 8> contexts;
   for (const auto &globalAsMember : GlobalsAsMembers) {
      contexts.push_back(globalAsMember.first);
   }
   llvm::array_pod_sort(contexts.begin(), contexts.end());

   // Collect all of the results in order.
   SmallVector<TypePHPLookupTable::SingleEntry, 4> results;
   for (const auto &context : contexts) {
      for (auto &entry : GlobalsAsMembers[context])
         results.push_back(mapStored(entry));
   }
   return results;
}

SmallVector<TypePHPLookupTable::SingleEntry, 4>
TypePHPLookupTable::lookup(SerializedTypePHPName baseName,
                           EffectiveClangContext searchContext) {
   // Translate context.
   Optional<StoredContext> context;
   if (searchContext) {
      context = translateContext(searchContext);
      if (!context) return { };
   }

   return lookup(baseName, context);
}

SmallVector<SerializedTypePHPName, 4> TypePHPLookupTable::allBaseNames() {
   // If we have a reader, enumerate its base names.
   if (Reader) return Reader->getBaseNames();

   // Otherwise, walk the lookup table.
   SmallVector<SerializedTypePHPName, 4> result;
   for (const auto &entry : LookupTable) {
      result.push_back(entry.first);
   }
   return result;
}

SmallVector<clang::NamedDecl *, 4>
TypePHPLookupTable::lookupObjCMembers(SerializedTypePHPName baseName) {
   SmallVector<clang::NamedDecl *, 4> result;

   // Find the lookup table entry for this base name.
   auto known = findOrCreate(baseName);
   if (known == LookupTable.end()) return result;

   // Walk each of the entries.
   for (auto &entry : known->second) {
      // If we're looking in a particular context and it doesn't match the
      // entry context, we're done.
      switch (entry.Context.first) {
         case ContextKind::TranslationUnit:
         case ContextKind::Tag:
            continue;

         case ContextKind::ObjCClass:
         case ContextKind::ObjCProtocol:
         case ContextKind::Typedef:
            break;
      }

      // Map each of the declarations.
      for (auto &stored : entry.DeclsOrMacros) {
         assert(isDeclEntry(stored) && "Not a declaration?");
         result.push_back(mapStoredDecl(stored));
      }
   }

   return result;
}

ArrayRef<clang::ObjCCategoryDecl *> TypePHPLookupTable::categories() {
   if (!Categories.empty() || !Reader) return Categories;

   // Map categories known to the reader.
   for (auto declID : Reader->categories()) {
      auto category =
         cast_or_null<clang::ObjCCategoryDecl>(
            Reader->getASTReader().GetLocalDecl(Reader->getModuleFile(), declID));
      if (category)
         Categories.push_back(category);

   }

   return Categories;
}

static void printName(clang::NamedDecl *named, llvm::raw_ostream &out) {
   // If there is a name, print it.
   if (!named->getDeclName().isEmpty()) {
      // If we have an Objective-C method, print the class name along
      // with '+'/'-'.
      if (auto objcMethod = dyn_cast<clang::ObjCMethodDecl>(named)) {
         out << (objcMethod->isInstanceMethod() ? '-' : '+') << '[';
         if (auto classDecl = objcMethod->getClassInterface()) {
            classDecl->printName(out);
            out << ' ';
         } else if (auto proto = dyn_cast<clang::ObjCProtocolDecl>(
            objcMethod->getDeclContext())) {
            proto->printName(out);
            out << ' ';
         }
         named->printName(out);
         out << ']';
         return;
      }

      // If we have an Objective-C property, print the class name along
      // with the property name.
      if (auto objcProperty = dyn_cast<clang::ObjCPropertyDecl>(named)) {
         auto dc = objcProperty->getDeclContext();
         if (auto classDecl = dyn_cast<clang::ObjCInterfaceDecl>(dc)) {
            classDecl->printName(out);
            out << '.';
         } else if (auto categoryDecl = dyn_cast<clang::ObjCCategoryDecl>(dc)) {
            categoryDecl->getClassInterface()->printName(out);
            out << '.';
         } else if (auto proto = dyn_cast<clang::ObjCProtocolDecl>(dc)) {
            proto->printName(out);
            out << '.';
         }
         named->printName(out);
         return;
      }

      named->printName(out);
      return;
   }

   // If this is an anonymous tag declaration with a typedef name, use that.
   if (auto tag = dyn_cast<clang::TagDecl>(named)) {
      if (auto typedefName = tag->getTypedefNameForAnonDecl()) {
         printName(typedefName, out);
         return;
      }
   }
}

void TypePHPLookupTable::deserializeAll() {
   if (!Reader) return;

   for (auto baseName : Reader->getBaseNames()) {
      (void)lookup(baseName, None);
   }

   (void)categories();

   for (auto context : Reader->getGlobalsAsMembersContexts()) {
      (void)lookupGlobalsAsMembers(context);
   }
}

/// Print a stored context to the given output stream for debugging purposes.
static void printStoredContext(TypePHPLookupTable::StoredContext context,
                               llvm::raw_ostream &out) {
   switch (context.first) {
      case TypePHPLookupTable::ContextKind::TranslationUnit:
         out << "TU";
         break;

      case TypePHPLookupTable::ContextKind::Tag:
      case TypePHPLookupTable::ContextKind::ObjCClass:
      case TypePHPLookupTable::ContextKind::ObjCProtocol:
      case TypePHPLookupTable::ContextKind::Typedef:
         out << context.second;
         break;
   }
}

static uint32_t getEncodedDeclID(uint64_t entry) {
   assert(TypePHPLookupTable::isSerializationIDEntry(entry));
   assert(TypePHPLookupTable::isDeclEntry(entry));
   return entry >> 2;
}

namespace {
struct LocalMacroIDs {
   uint32_t moduleID;
   uint32_t nameOrMacroID;
};
}

static LocalMacroIDs getEncodedModuleMacroIDs(uint64_t entry) {
   assert(TypePHPLookupTable::isSerializationIDEntry(entry));
   assert(TypePHPLookupTable::isMacroEntry(entry));
   return {static_cast<uint32_t>((entry & 0xFFFFFFFF) >> 2),
           static_cast<uint32_t>(entry >> 32)};
}

/// Print a stored entry (Clang macro or declaration) for debugging purposes.
static void printStoredEntry(const TypePHPLookupTable *table, uint64_t entry,
                             llvm::raw_ostream &out) {
   if (TypePHPLookupTable::isSerializationIDEntry(entry)) {
      if (TypePHPLookupTable::isDeclEntry(entry)) {
         llvm::errs() << "decl ID #" << getEncodedDeclID(entry);
      } else {
         LocalMacroIDs macroIDs = getEncodedModuleMacroIDs(entry);
         if (macroIDs.moduleID == 0) {
            llvm::errs() << "macro ID #" << macroIDs.nameOrMacroID;
         } else {
            llvm::errs() << "macro with name ID #" << macroIDs.nameOrMacroID
                         << "in submodule #" << macroIDs.moduleID;
         }
      }
   } else if (TypePHPLookupTable::isMacroEntry(entry)) {
      llvm::errs() << "Macro";
   } else {
      auto decl = const_cast<TypePHPLookupTable *>(table)->mapStoredDecl(entry);
      printName(decl, llvm::errs());
   }
}

void TypePHPLookupTable::dump() const {
   dump(llvm::errs());
}

void TypePHPLookupTable::dump(raw_ostream &os) const {
   // Dump the base name -> full table entry mappings.
   SmallVector<SerializedTypePHPName, 4> baseNames;
   for (const auto &entry : LookupTable) {
      baseNames.push_back(entry.first);
   }
   llvm::array_pod_sort(baseNames.begin(), baseNames.end());
   os << "Base name -> entry mappings:\n";
   for (auto baseName : baseNames) {
      switch (baseName.Kind) {
         case DeclBaseName::Kind::Normal:
            os << "  " << baseName.Name << ":\n";
            break;
         case DeclBaseName::Kind::Subscript:
            os << "  subscript:\n";
            break;
         case DeclBaseName::Kind::Constructor:
            os << "  init:\n";
            break;
         case DeclBaseName::Kind::Destructor:
            os << "  deinit:\n";
            break;
      }
      const auto &entries = LookupTable.find(baseName)->second;
      for (const auto &entry : entries) {
         os << "    ";
         printStoredContext(entry.Context, os);
         os << ": ";

         interleave(entry.DeclsOrMacros.begin(), entry.DeclsOrMacros.end(),
                    [this, &os](uint64_t entry) {
                       printStoredEntry(this, entry, os);
                    },
                    [&os] {
                       os << ", ";
                    });
         os << "\n";
      }
   }

   if (!Categories.empty()) {
      os << "Categories: ";
      interleave(Categories.begin(), Categories.end(),
                 [&os](clang::ObjCCategoryDecl *category) {
                    os << category->getClassInterface()->getName()
                       << "(" << category->getName() << ")";
                 },
                 [&os] {
                    os << ", ";
                 });
      os << "\n";
   } else if (Reader && !Reader->categories().empty()) {
      os << "Categories: ";
      interleave(Reader->categories().begin(), Reader->categories().end(),
                 [&os](clang::serialization::DeclID declID) {
                    os << "decl ID #" << declID;
                 },
                 [&os] {
                    os << ", ";
                 });
      os << "\n";
   }

   if (!GlobalsAsMembers.empty()) {
      os << "Globals-as-members mapping:\n";
      SmallVector<StoredContext, 4> contexts;
      for (const auto &entry : GlobalsAsMembers) {
         contexts.push_back(entry.first);
      }
      llvm::array_pod_sort(contexts.begin(), contexts.end());
      for (auto context : contexts) {
         os << "  ";
         printStoredContext(context, os);
         os << ": ";

         const auto &entries = GlobalsAsMembers.find(context)->second;
         interleave(entries.begin(), entries.end(),
                    [this, &os](uint64_t entry) {
                       printStoredEntry(this, entry, os);
                    },
                    [&os] {
                       os << ", ";
                    });
         os << "\n";
      }
   }
}

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------

namespace {
//enum RecordTypes {
//   /// Record that contains the mapping from base names to entities with that
//   /// name.
//      BASE_NAME_TO_ENTITIES_RECORD_ID
//      = clang::serialization::FIRST_EXTENSION_RECORD_ID,
//
//   /// Record that contains the list of Objective-C category/extension IDs.
//      CATEGORIES_RECORD_ID,
//
//   /// Record that contains the mapping from contexts to the list of
//   /// globals that will be injected as members into those contexts.
//      GLOBALS_AS_MEMBERS_RECORD_ID
//};
//
//using BaseNameToEntitiesTableRecordLayout
//= BCRecordLayout<BASE_NAME_TO_ENTITIES_RECORD_ID, BCVBR<16>, BCBlob>;
//
//using CategoriesRecordLayout
//= llvm::BCRecordLayout<CATEGORIES_RECORD_ID, BCBlob>;
//
//using GlobalsAsMembersTableRecordLayout
//= BCRecordLayout<GLOBALS_AS_MEMBERS_RECORD_ID, BCVBR<16>, BCBlob>;

/// Trait used to write the on-disk hash table for the base name -> entities
/// mapping.
class BaseNameToEntitiesTableWriterInfo {
   static_assert(sizeof(DeclBaseName::Kind) <= sizeof(uint8_t),
                 "kind serialized as uint8_t");

   TypePHPLookupTable &Table;
   clang::ASTWriter &Writer;

public:
   using key_type = SerializedTypePHPName;
   using key_type_ref = key_type;
   using data_type = SmallVector<TypePHPLookupTable::FullTableEntry, 2>;
   using data_type_ref = data_type &;
   using hash_value_type = uint32_t;
   using offset_type = unsigned;

   BaseNameToEntitiesTableWriterInfo(TypePHPLookupTable &table,
                                     clang::ASTWriter &writer)
      : Table(table), Writer(writer)
   {
   }

   hash_value_type ComputeHash(key_type_ref key) {
      return llvm::DenseMapInfo<SerializedTypePHPName>::getHashValue(key);
   }

   std::pair<unsigned, unsigned> EmitKeyDataLength(raw_ostream &out,
                                                   key_type_ref key,
                                                   data_type_ref data) {
      uint32_t keyLength = sizeof(uint8_t); // For the flag of the name's kind
      if (key.Kind == DeclBaseName::Kind::Normal) {
         keyLength += key.Name.size(); // The name's length
      }
      assert(keyLength == static_cast<uint16_t>(keyLength));

      // # of entries
      uint32_t dataLength = sizeof(uint16_t);

      // Storage per entry.
      for (const auto &entry : data) {
         // Context info.
         dataLength += 1;
         if (TypePHPLookupTable::contextRequiresName(entry.Context.first)) {
            dataLength += sizeof(uint16_t) + entry.Context.second.size();
         }

         // # of entries.
         dataLength += sizeof(uint16_t);

         // Actual entries.
         dataLength += (sizeof(uint64_t) * entry.DeclsOrMacros.size());
      }

      endian::Writer writer(out, little);
      writer.write<uint16_t>(keyLength);
      writer.write<uint32_t>(dataLength);
      return { keyLength, dataLength };
   }

   void EmitKey(raw_ostream &out, key_type_ref key, unsigned len) {
      endian::Writer writer(out, little);
      writer.write<uint8_t>((uint8_t)key.Kind);
      if (key.Kind == polar::DeclBaseName::Kind::Normal)
         writer.OS << key.Name;
   }

   void EmitData(raw_ostream &out, key_type_ref key, data_type_ref data,
                 unsigned len) {
      endian::Writer writer(out, little);

      // # of entries
      writer.write<uint16_t>(data.size());
      assert(data.size() == static_cast<uint16_t>(data.size()));

      bool isModule = Writer.getLangOpts().isCompilingModule();
      for (auto &fullEntry : data) {
         // Context.
         writer.write<uint8_t>(static_cast<uint8_t>(fullEntry.Context.first));
         if (TypePHPLookupTable::contextRequiresName(fullEntry.Context.first)) {
            writer.write<uint16_t>(fullEntry.Context.second.size());
            out << fullEntry.Context.second;
         }

         // # of entries.
         writer.write<uint16_t>(fullEntry.DeclsOrMacros.size());

         // Write the declarations and macros.
         for (auto &entry : fullEntry.DeclsOrMacros) {
            uint64_t id;
            auto mappedEntry = Table.mapStored(entry, isModule);
            if (auto *decl = mappedEntry.dyn_cast<clang::NamedDecl *>()) {
               id = (Writer.getDeclID(decl) << 2) | 0x02;
            } else if (auto *macro = mappedEntry.dyn_cast<clang::MacroInfo *>()) {
               id = static_cast<uint64_t>(Writer.getMacroID(macro)) << 32;
               id |= 0x02 | 0x01;
            } else {
               auto *moduleMacro = mappedEntry.get<clang::ModuleMacro *>();
               uint32_t nameID = Writer.getIdentifierRef(moduleMacro->getName());
               uint32_t submoduleID = Writer.getLocalOrImportedSubmoduleID(
                  moduleMacro->getOwningModule());
               id = (static_cast<uint64_t>(nameID) << 32) | (submoduleID << 2);
               id |= 0x02 | 0x01;
            }
            writer.write<uint64_t>(id);
         }
      }
   }
};

/// Trait used to write the on-disk hash table for the
/// globals-as-members mapping.
class GlobalsAsMembersTableWriterInfo {
   TypePHPLookupTable &Table;
   clang::ASTWriter &Writer;

public:
   using key_type = std::pair<TypePHPLookupTable::ContextKind, StringRef>;
   using key_type_ref = key_type;
   using data_type = SmallVector<uint64_t, 2>;
   using data_type_ref = data_type &;
   using hash_value_type = uint32_t;
   using offset_type = unsigned;

   GlobalsAsMembersTableWriterInfo(TypePHPLookupTable &table,
                                   clang::ASTWriter &writer)
      : Table(table), Writer(writer)
   {
   }

   hash_value_type ComputeHash(key_type_ref key) {
      return static_cast<unsigned>(key.first) + llvm::djbHash(key.second);
   }

   std::pair<unsigned, unsigned> EmitKeyDataLength(raw_ostream &out,
                                                   key_type_ref key,
                                                   data_type_ref data) {
      // The length of the key.
      uint32_t keyLength = 1;
      if (TypePHPLookupTable::contextRequiresName(key.first))
         keyLength += key.second.size();
      assert(keyLength == static_cast<uint16_t>(keyLength));

      // # of entries
      uint32_t dataLength =
         sizeof(uint16_t) + sizeof(uint64_t) * data.size();
      assert(dataLength == static_cast<uint16_t>(dataLength));

      endian::Writer writer(out, little);
      writer.write<uint16_t>(keyLength);
      writer.write<uint16_t>(dataLength);
      return { keyLength, dataLength };
   }

   void EmitKey(raw_ostream &out, key_type_ref key, unsigned len) {
      endian::Writer writer(out, little);
      writer.write<uint8_t>(static_cast<unsigned>(key.first) - 2);
      if (TypePHPLookupTable::contextRequiresName(key.first))
         out << key.second;
   }

   void EmitData(raw_ostream &out, key_type_ref key, data_type_ref data,
                 unsigned len) {
      endian::Writer writer(out, little);

      // # of entries
      writer.write<uint16_t>(data.size());

      // Actual entries.
      bool isModule = Writer.getLangOpts().isCompilingModule();
      for (auto &entry : data) {
         uint64_t id;
         auto mappedEntry = Table.mapStored(entry, isModule);
         if (auto *decl = mappedEntry.dyn_cast<clang::NamedDecl *>()) {
            id = (Writer.getDeclID(decl) << 2) | 0x02;
         } else if (auto *macro = mappedEntry.dyn_cast<clang::MacroInfo *>()) {
            id = static_cast<uint64_t>(Writer.getMacroID(macro)) << 32;
            id |= 0x02 | 0x01;
         } else {
            auto *moduleMacro = mappedEntry.get<clang::ModuleMacro *>();
            uint32_t nameID = Writer.getIdentifierRef(moduleMacro->getName());
            uint32_t submoduleID = Writer.getLocalOrImportedSubmoduleID(
               moduleMacro->getOwningModule());
            id = (static_cast<uint64_t>(nameID) << 32) | (submoduleID << 2);
            id |= 0x02 | 0x01;
         }
         writer.write<uint64_t>(id);
      }
   }
};
} // end anonymous namespace

void TypePHPLookupTableWriter::writeExtensionContents(
   clang::Sema &sema,
   llvm::BitstreamWriter &stream) {
   NameImporter nameImporter(typePHPCtx, availability, sema, inferImportAsMember);

   // Populate the lookup table.
   TypePHPLookupTable table(nullptr);
   populateTable(table, nameImporter);

   SmallVector<uint64_t, 64> ScratchRecord;

   // First, gather the sorted list of base names.
   SmallVector<SerializedTypePHPName, 2> baseNames;
   for (const auto &entry : table.LookupTable)
      baseNames.push_back(entry.first);
   llvm::array_pod_sort(baseNames.begin(), baseNames.end());

   // Form the mapping from base names to entities with their context.
   {
      llvm::SmallString<4096> hashTableBlob;
      uint32_t tableOffset;
      {
         llvm::OnDiskChainedHashTableGenerator<BaseNameToEntitiesTableWriterInfo>
            generator;
         BaseNameToEntitiesTableWriterInfo info(table, Writer);
         for (auto baseName : baseNames)
            generator.insert(baseName, table.LookupTable[baseName], info);

         llvm::raw_svector_ostream blobStream(hashTableBlob);
         // Make sure that no bucket is at offset 0
         endian::write<uint32_t>(blobStream, 0, little);
         tableOffset = generator.Emit(blobStream, info);
      }
      // @todo
//      BaseNameToEntitiesTableRecordLayout layout(stream);
//      layout.emit(ScratchRecord, tableOffset, hashTableBlob);
   }

   // Write the categories, if there are any.
   if (!table.Categories.empty()) {
      SmallVector<clang::serialization::DeclID, 4> categoryIDs;
      for (auto category : table.Categories) {
         categoryIDs.push_back(Writer.getDeclID(category));
      }

      StringRef blob(reinterpret_cast<const char *>(categoryIDs.data()),
                     categoryIDs.size() * sizeof(clang::serialization::DeclID));
      // @todo
//      CategoriesRecordLayout layout(stream);
//      layout.emit(ScratchRecord, blob);
   }

   // Write the globals-as-members table, if non-empty.
   if (!table.GlobalsAsMembers.empty()) {
      // Sort the keys.
      SmallVector<TypePHPLookupTable::StoredContext, 4> contexts;
      for (const auto &entry : table.GlobalsAsMembers) {
         contexts.push_back(entry.first);
      }
      llvm::array_pod_sort(contexts.begin(), contexts.end());

      // Create the on-disk hash table.
      llvm::SmallString<4096> hashTableBlob;
      uint32_t tableOffset;
      {
         llvm::OnDiskChainedHashTableGenerator<GlobalsAsMembersTableWriterInfo>
            generator;
         GlobalsAsMembersTableWriterInfo info(table, Writer);
         for (auto context : contexts)
            generator.insert(context, table.GlobalsAsMembers[context], info);

         llvm::raw_svector_ostream blobStream(hashTableBlob);
         // Make sure that no bucket is at offset 0
         endian::write<uint32_t>(blobStream, 0, little);
         tableOffset = generator.Emit(blobStream, info);
      }

//      GlobalsAsMembersTableRecordLayout layout(stream);
//      layout.emit(ScratchRecord, tableOffset, hashTableBlob);
   }
}

namespace {
/// Used to deserialize the on-disk base name -> entities table.
class BaseNameToEntitiesTableReaderInfo {
public:
   using internal_key_type = SerializedTypePHPName;
   using external_key_type = internal_key_type;
   using data_type = SmallVector<TypePHPLookupTable::FullTableEntry, 2>;
   using hash_value_type = uint32_t;
   using offset_type = unsigned;

   internal_key_type GetInternalKey(external_key_type key) {
      return key;
   }

   external_key_type GetExternalKey(internal_key_type key) {
      return key;
   }

   hash_value_type ComputeHash(internal_key_type key) {
      return llvm::DenseMapInfo<SerializedTypePHPName>::getHashValue(key);
   }

   static bool EqualKey(internal_key_type lhs, internal_key_type rhs) {
      return lhs == rhs;
   }

   static std::pair<unsigned, unsigned>
   ReadKeyDataLength(const uint8_t *&data) {
      unsigned keyLength = endian::readNext<uint16_t, little, unaligned>(data);
      unsigned dataLength = endian::readNext<uint32_t, little, unaligned>(data);
      return { keyLength, dataLength };
   }

   static internal_key_type ReadKey(const uint8_t *data, unsigned length) {
      uint8_t kind = endian::readNext<uint8_t, little, unaligned>(data);
      switch (kind) {
         case (uint8_t)DeclBaseName::Kind::Normal: {
            StringRef str(reinterpret_cast<const char *>(data),
                          length - sizeof(uint8_t));
            return SerializedTypePHPName(str);
         }
         case (uint8_t)DeclBaseName::Kind::Subscript:
            return SerializedTypePHPName(DeclBaseName::Kind::Subscript);
         case (uint8_t)DeclBaseName::Kind::Constructor:
            return SerializedTypePHPName(DeclBaseName::Kind::Constructor);
         case (uint8_t)DeclBaseName::Kind::Destructor:
            return SerializedTypePHPName(DeclBaseName::Kind::Destructor);
         default:
            llvm_unreachable("Unknown kind for DeclBaseName");
      }
   }

   static data_type ReadData(internal_key_type key, const uint8_t *data,
                             unsigned length) {
      data_type result;

      // # of entries.
      unsigned numEntries = endian::readNext<uint16_t, little, unaligned>(data);
      result.reserve(numEntries);

      // Read all of the entries.
      while (numEntries--) {
         TypePHPLookupTable::FullTableEntry entry;

         // Read the context.
         entry.Context.first =
            static_cast<TypePHPLookupTable::ContextKind>(
               endian::readNext<uint8_t, little, unaligned>(data));
         if (TypePHPLookupTable::contextRequiresName(entry.Context.first)) {
            uint16_t length = endian::readNext<uint16_t, little, unaligned>(data);
            entry.Context.second = StringRef((const char *)data, length);
            data += length;
         }

         // Read the declarations and macros.
         unsigned numDeclsOrMacros =
            endian::readNext<uint16_t, little, unaligned>(data);
         while (numDeclsOrMacros--) {
            auto id = endian::readNext<uint64_t, little, unaligned>(data);
            entry.DeclsOrMacros.push_back(id);
         }

         result.push_back(entry);
      }

      return result;
   }
};

/// Used to deserialize the on-disk globals-as-members table.
class GlobalsAsMembersTableReaderInfo {
public:
   using internal_key_type = TypePHPLookupTable::StoredContext;
   using external_key_type = internal_key_type;
   using data_type = SmallVector<uint64_t, 2>;
   using hash_value_type = uint32_t;
   using offset_type = unsigned;

   internal_key_type GetInternalKey(external_key_type key) {
      return key;
   }

   external_key_type GetExternalKey(internal_key_type key) {
      return key;
   }

   hash_value_type ComputeHash(internal_key_type key) {
      return static_cast<unsigned>(key.first) + llvm::djbHash(key.second);
   }

   static bool EqualKey(internal_key_type lhs, internal_key_type rhs) {
      return lhs == rhs;
   }

   static std::pair<unsigned, unsigned>
   ReadKeyDataLength(const uint8_t *&data) {
      unsigned keyLength = endian::readNext<uint16_t, little, unaligned>(data);
      unsigned dataLength = endian::readNext<uint16_t, little, unaligned>(data);
      return { keyLength, dataLength };
   }

   static internal_key_type ReadKey(const uint8_t *data, unsigned length) {
      return internal_key_type(
         static_cast<TypePHPLookupTable::ContextKind>(*data + 2),
         StringRef((const char *)data + 1, length - 1));
   }

   static data_type ReadData(internal_key_type key, const uint8_t *data,
                             unsigned length) {
      data_type result;

      // # of entries.
      unsigned numEntries = endian::readNext<uint16_t, little, unaligned>(data);
      result.reserve(numEntries);

      // Read all of the entries.
      while (numEntries--) {
         auto id = endian::readNext<uint64_t, little, unaligned>(data);
         result.push_back(id);
      }

      return result;
   }
};
} // end anonymous namespace

clang::NamedDecl *TypePHPLookupTable::mapStoredDecl(uint64_t &entry) {
   assert(isDeclEntry(entry) && "Not a declaration entry");

   // If we have an AST node here, just cast it.
   if (isASTNodeEntry(entry)) {
      return static_cast<clang::NamedDecl *>(getPointerFromEntry(entry));
   }

   // Otherwise, resolve the declaration.
   assert(Reader && "Cannot resolve the declaration without a reader");
   uint32_t declID = getEncodedDeclID(entry);
   auto decl = cast_or_null<clang::NamedDecl>(
      Reader->getASTReader().GetLocalDecl(Reader->getModuleFile(),
                                          declID));

   // Update the entry now that we've resolved the declaration.
   entry = encodeEntry(decl);
   return decl;
}

static bool isPCH(TypePHPLookupTableReader &reader) {
   return reader.getModuleFile().Kind == clang::serialization::MK_PCH;
}

TypePHPLookupTable::SingleEntry
TypePHPLookupTable::mapStoredMacro(uint64_t &entry, bool assumeModule) {
   assert(isMacroEntry(entry) && "Not a macro entry");

   // If we have an AST node here, just cast it.
   if (isASTNodeEntry(entry)) {
      if (assumeModule || (Reader && !isPCH(*Reader)))
         return static_cast<clang::ModuleMacro *>(getPointerFromEntry(entry));
      else
         return static_cast<clang::MacroInfo *>(getPointerFromEntry(entry));
   }

   // Otherwise, resolve the macro.
   assert(Reader && "Cannot resolve the macro without a reader");
   clang::ASTReader &astReader = Reader->getASTReader();

   LocalMacroIDs macroIDs = getEncodedModuleMacroIDs(entry);
   if (!assumeModule && macroIDs.moduleID == 0) {
      assert(isPCH(*Reader));
      // Not a module, and the second key is actually a macroID.
      auto macro =
         astReader.getMacro(astReader.getGlobalMacroID(Reader->getModuleFile(),
                                                       macroIDs.nameOrMacroID));

      // Update the entry now that we've resolved the macro.
      entry = encodeEntry(macro);
      return macro;
   }

   // FIXME: Clang should help us out here, but it doesn't. It can only give us
   // MacroInfos and not ModuleMacros.
   assert(!isPCH(*Reader));
   clang::IdentifierInfo *name =
      astReader.getLocalIdentifier(Reader->getModuleFile(),
                                   macroIDs.nameOrMacroID);
   auto submoduleID = astReader.getGlobalSubmoduleID(Reader->getModuleFile(),
                                                     macroIDs.moduleID);
   clang::Module *submodule = astReader.getSubmodule(submoduleID);
   assert(submodule);

   clang::Preprocessor &pp = Reader->getASTReader().getPreprocessor();
   // Force the ModuleMacro to be loaded if this module is visible.
   (void)pp.getLeafModuleMacros(name);
   clang::ModuleMacro *macro = pp.getModuleMacro(submodule, name);
   // This might still be NULL if the module has been imported but not made
   // visible. We need a better answer here.
   if (macro)
      entry = encodeEntry(macro);
   return macro;
}

TypePHPLookupTable::SingleEntry TypePHPLookupTable::mapStored(uint64_t &entry,
                                                              bool assumeModule) {
   if (isDeclEntry(entry))
      return mapStoredDecl(entry);
   return mapStoredMacro(entry, assumeModule);
}

TypePHPLookupTableReader::~TypePHPLookupTableReader() {
   OnRemove();
}

std::unique_ptr<TypePHPLookupTableReader>
TypePHPLookupTableReader::create(clang::ModuleFileExtension *extension,
                                 clang::ASTReader &reader,
                                 clang::serialization::ModuleFile &moduleFile,
                                 std::function<void()> onRemove,
                                 const llvm::BitstreamCursor &stream)
{
   // Look for the base name -> entities table record.
   SmallVector<uint64_t, 64> scratch;
   auto cursor = stream;
   llvm::Expected<llvm::BitstreamEntry> maybeNext = cursor.advance();
   if (!maybeNext) {
      // FIXME this drops the error on the floor.
      consumeError(maybeNext.takeError());
      return nullptr;
   }
   llvm::BitstreamEntry next = maybeNext.get();
   std::unique_ptr<SerializedBaseNameToEntitiesTable> serializedTable;
   std::unique_ptr<SerializedGlobalsAsMembersTable> globalsAsMembersTable;
   ArrayRef<clang::serialization::DeclID> categories;
   while (next.Kind != llvm::BitstreamEntry::EndBlock) {
      if (next.Kind == llvm::BitstreamEntry::Error)
         return nullptr;

      if (next.Kind == llvm::BitstreamEntry::SubBlock) {
         // Unknown sub-block, possibly for use by a future version of the
         // API notes format.
         if (cursor.SkipBlock())
            return nullptr;

         maybeNext = cursor.advance();
         if (!maybeNext) {
            // FIXME this drops the error on the floor.
            consumeError(maybeNext.takeError());
            return nullptr;
         }
         next = maybeNext.get();
         continue;
      }

      scratch.clear();
      StringRef blobData;
      llvm::Expected<unsigned> maybeKind =
         cursor.readRecord(next.ID, scratch, &blobData);
      if (!maybeKind) {
         // FIXME this drops the error on the floor.
         consumeError(maybeNext.takeError());
         return nullptr;
      }
      // @todo
//      unsigned kind = maybeKind.get();
//      switch (kind) {
//         case BASE_NAME_TO_ENTITIES_RECORD_ID: {
//            // Already saw base name -> entities table.
//            if (serializedTable)
//               return nullptr;
//
//            uint32_t tableOffset;
//            BaseNameToEntitiesTableRecordLayout::readRecord(scratch, tableOffset);
//            auto base = reinterpret_cast<const uint8_t *>(blobData.data());
//
//            serializedTable.reset(
//               SerializedBaseNameToEntitiesTable::Create(base + tableOffset,
//                                                         base + sizeof(uint32_t),
//                                                         base));
//            break;
//         }
//
//         case CATEGORIES_RECORD_ID: {
//            // Already saw categories; input is malformed.
//            if (!categories.empty()) return nullptr;
//
//            auto start =
//               reinterpret_cast<const clang::serialization::DeclID *>(blobData.data());
//            unsigned numElements
//               = blobData.size() / sizeof(clang::serialization::DeclID);
//            categories = llvm::makeArrayRef(start, numElements);
//            break;
//         }
//
//         case GLOBALS_AS_MEMBERS_RECORD_ID: {
//            // Already saw globals-as-members table.
//            if (globalsAsMembersTable)
//               return nullptr;
//
//            uint32_t tableOffset;
//            GlobalsAsMembersTableRecordLayout::readRecord(scratch, tableOffset);
//            auto base = reinterpret_cast<const uint8_t *>(blobData.data());
//
//            globalsAsMembersTable.reset(
//               SerializedGlobalsAsMembersTable::Create(base + tableOffset,
//                                                       base + sizeof(uint32_t),
//                                                       base));
//            break;
//         }
//
//         default:
//            // Unknown record, possibly for use by a future version of the
//            // module format.
//            break;
//      }

      maybeNext = cursor.advance();
      if (!maybeNext) {
         // FIXME this drops the error on the floor.
         consumeError(maybeNext.takeError());
         return nullptr;
      }
      next = maybeNext.get();
   }

   if (!serializedTable) return nullptr;

   // Create the reader.
   // Note: This doesn't use llvm::make_unique because the constructor is
   // private.
   return std::unique_ptr<TypePHPLookupTableReader>(
      new TypePHPLookupTableReader(extension, reader, moduleFile, onRemove,
                                   std::move(serializedTable), categories,
                                   std::move(globalsAsMembersTable)));

}

SmallVector<SerializedTypePHPName, 4> TypePHPLookupTableReader::getBaseNames() {
   SmallVector<SerializedTypePHPName, 4> results;
   for (auto key : SerializedTable->keys()) {
      results.push_back(key);
   }
   return results;
}

bool TypePHPLookupTableReader::lookup(
   SerializedTypePHPName baseName,
   SmallVectorImpl<TypePHPLookupTable::FullTableEntry> &entries) {
   // Look for an entry with this base name.
   auto known = SerializedTable->find(baseName);
   if (known == SerializedTable->end()) return false;

   // Grab the results.
   entries = std::move(*known);
   return true;
}

SmallVector<TypePHPLookupTable::StoredContext, 4>
TypePHPLookupTableReader::getGlobalsAsMembersContexts() {
   SmallVector<TypePHPLookupTable::StoredContext, 4> results;
   if (!GlobalsAsMembersTable) return results;

   for (auto key : GlobalsAsMembersTable->keys()) {
      results.push_back(key);
   }
   return results;
}

bool TypePHPLookupTableReader::lookupGlobalsAsMembers(
   TypePHPLookupTable::StoredContext context,
   SmallVectorImpl<uint64_t> &entries) {
   if (!GlobalsAsMembersTable) return false;

   // Look for an entry with this context name.
   auto known = GlobalsAsMembersTable->find(context);
   if (known == GlobalsAsMembersTable->end()) return false;

   // Grab the results.
   entries = std::move(*known);
   return true;
}

clang::ModuleFileExtensionMetadata
TypePHPNameLookupExtension::getExtensionMetadata() const {
   clang::ModuleFileExtensionMetadata metadata;
   metadata.BlockName = "typephp.lookup";
   metadata.MajorVersion = POLAR_LOOKUP_TABLE_VERSION_MAJOR;
   metadata.MinorVersion = POLAR_LOOKUP_TABLE_VERSION_MINOR;
   metadata.UserInfo =
      version::retrieve_polarphp_full_version(typePHPCtx.LangOpts.EffectiveLanguageVersion);
   return metadata;
}

llvm::hash_code
TypePHPNameLookupExtension::hashExtension(llvm::hash_code code) const {
   return llvm::hash_combine(code, StringRef("typephp.lookup"),
                             POLAR_LOOKUP_TABLE_VERSION_MAJOR,
                             POLAR_LOOKUP_TABLE_VERSION_MINOR,
                             inferImportAsMember,
                             version::retrieve_polarphp_full_version());
}

void importer::addEntryToLookupTable(TypePHPLookupTable &table,
                                     clang::NamedDecl *named,
                                     NameImporter &nameImporter) {
   // Determine whether this declaration is suppressed in Swift.
   if (shouldSuppressDeclImport(named))
      return;

   // Leave incomplete struct/enum/union types out of the table; Swift only
   // handles pointers to them.
   // FIXME: At some point we probably want to be importing incomplete types,
   // so that pointers to different incomplete types themselves have distinct
   // types. At that time it will be necessary to make the decision of whether
   // or not to import an incomplete type declaration based on whether it's
   // actually the struct backing a CF type:
   //
   //    typedef struct CGColor *CGColorRef;
   //
   // The best way to do this is probably to change CFDatabase.def to include
   // struct names when relevant, not just pointer names. That way we can check
   // both CFDatabase.def and the objc_bridge attribute and cover all our bases.
   if (auto *tagDecl = dyn_cast<clang::TagDecl>(named)) {
      if (!tagDecl->getDefinition())
         return;
   }

   // If we have a name to import as, add this entry to the table.
   auto currentVersion =
      ImportNameVersion::fromOptions(nameImporter.getLangOpts());
   auto failed = nameImporter.forEachDistinctImportName(
      named, currentVersion,
      [&](ImportedName importedName, ImportNameVersion version) {
         table.addEntry(importedName.getDeclName(), named,
                        importedName.getEffectiveContext());

         // Also add the subscript entry, if needed.
         if (version == currentVersion && importedName.isSubscriptAccessor()) {
            table.addEntry(DeclName(nameImporter.getContext(),
                                    DeclBaseName::createSubscript(),
                                    {Identifier()}),
                           named, importedName.getEffectiveContext());
         }

         return true;
      });
   if (failed) {
      if (auto category = dyn_cast<clang::ObjCCategoryDecl>(named)) {
         // If the category is invalid, don't add it.
         if (category->isInvalidDecl())
            return;

         table.addCategory(category);
      }
   }

   // Walk the members of any context that can have nested members.
   if (isa<clang::TagDecl>(named) || isa<clang::ObjCInterfaceDecl>(named) ||
       isa<clang::ObjCProtocolDecl>(named) ||
       isa<clang::ObjCCategoryDecl>(named) || isa<clang::NamespaceDecl>(named)) {
      clang::DeclContext *dc = cast<clang::DeclContext>(named);
      for (auto member : dc->decls()) {
         if (auto namedMember = dyn_cast<clang::NamedDecl>(member))
            addEntryToLookupTable(table, namedMember, nameImporter);
      }
   }
}

/// Returns the nearest parent of \p module that is marked \c explicit in its
/// module map. If \p module is itself explicit, it is returned; if no module
/// in the parent chain is explicit, the top-level module is returned.
static const clang::Module *
getExplicitParentModule(const clang::Module *module) {
   while (!module->IsExplicit && module->Parent)
      module = module->Parent;
   return module;
}

void importer::addMacrosToLookupTable(TypePHPLookupTable &table,
                                      NameImporter &nameImporter) {
   auto &pp = nameImporter.getClangPreprocessor();
   auto *tu = nameImporter.getClangContext().getTranslationUnitDecl();
   bool isModule = pp.getLangOpts().isCompilingModule();
   for (const auto &macro : pp.macros(false)) {
      auto maybeAddMacro = [&](clang::MacroInfo *info,
                               clang::ModuleMacro *moduleMacro) {
         // If this is a #undef, return.
         if (!info)
            return;

         // If we hit a builtin macro, we're done.
         if (info->isBuiltinMacro())
            return;

         // If we hit a macro with invalid or predefined location, we're done.
         auto loc = info->getDefinitionLoc();
         if (loc.isInvalid())
            return;
         if (pp.getSourceManager().getFileID(loc) == pp.getPredefinesFileID())
            return;

         // If we're in a module, we really need moduleMacro to be valid.
         if (isModule && !moduleMacro) {
#ifndef NDEBUG
            // Refetch this just for the assertion.
            clang::MacroDirective *MD = pp.getLocalMacroDirective(macro.first);
            assert(isa<clang::VisibilityMacroDirective>(MD));
#endif

            // FIXME: "public" visibility macros should actually be added to the
            // table.
            return;
         }

         // Add this entry.
         auto name = nameImporter.importMacroName(macro.first, info);
         if (name.empty())
            return;
         if (moduleMacro)
            table.addEntry(name, moduleMacro, tu);
         else
            table.addEntry(name, info, tu);
      };

      ArrayRef<clang::ModuleMacro *> moduleMacros =
         macro.second.getActiveModuleMacros(pp, macro.first);
      if (moduleMacros.empty()) {
         // Handle the bridging header case.
         clang::MacroDirective *MD = pp.getLocalMacroDirective(macro.first);
         if (!MD)
            continue;

         maybeAddMacro(MD->getMacroInfo(), nullptr);

      } else {
         clang::Module *currentModule = pp.getCurrentModule();
         SmallVector<clang::ModuleMacro *, 8> worklist;
         llvm::copy_if(moduleMacros, std::back_inserter(worklist),
                       [currentModule](const clang::ModuleMacro *next) -> bool {
                          return next->getOwningModule()->isSubModuleOf(currentModule);
                       });

         while (!worklist.empty()) {
            clang::ModuleMacro *moduleMacro = worklist.pop_back_val();
            maybeAddMacro(moduleMacro->getMacroInfo(), moduleMacro);

            // Also visit overridden macros that are in a different explicit
            // submodule. This isn't a perfect way to tell if these two macros are
            // supposed to be independent, but it's close enough in practice.
            clang::Module *owningModule = moduleMacro->getOwningModule();
            auto *explicitParent = getExplicitParentModule(owningModule);
            llvm::copy_if(moduleMacro->overrides(), std::back_inserter(worklist),
                          [&](const clang::ModuleMacro *next) -> bool {
                             const clang::Module *nextModule =
                                getExplicitParentModule(next->getOwningModule());
                             if (!nextModule->isSubModuleOf(currentModule))
                                return false;
                             return nextModule != explicitParent;
                          });
         }
      }
   }
}

void importer::finalizeLookupTable(
   TypePHPLookupTable &table, NameImporter &nameImporter,
   ClangSourceBufferImporter &buffersForDiagnostics) {
   // Resolve any unresolved entries.
   SmallVector<TypePHPLookupTable::SingleEntry, 4> unresolved;
   if (table.resolveUnresolvedEntries(unresolved)) {
      // Complain about unresolved entries that remain.
      for (auto entry : unresolved) {
         auto decl = entry.get<clang::NamedDecl *>();
         // @todo
//         auto swiftName = decl->getAttr<clang::SwiftNameAttr>();
//
//         if (swiftName) {
//            clang::SourceLocation diagLoc = swiftName->getLocation();
//            if (!diagLoc.isValid())
//               diagLoc = decl->getLocation();
//            SourceLoc swiftSourceLoc = buffersForDiagnostics.resolveSourceLocation(
//               nameImporter.getClangContext().getSourceManager(), diagLoc);
//
//            DiagnosticEngine &swiftDiags = nameImporter.getContext().Diags;
//            swiftDiags.diagnose(swiftSourceLoc, diag::unresolvable_clang_decl,
//                                decl->getNameAsString(), swiftName->getName());
//            StringRef moduleName =
//               nameImporter.getClangContext().getLangOpts().CurrentModule;
//            if (!moduleName.empty()) {
//               swiftDiags.diagnose(swiftSourceLoc,
//                                   diag::unresolvable_clang_decl_is_a_framework_bug,
//                                   moduleName);
//            }
//         }
      }
   }
}

void TypePHPLookupTableWriter::populateTableWithDecl(TypePHPLookupTable &table,
                                                     NameImporter &nameImporter,
                                                     clang::Decl *decl) {
   // Skip anything from an AST file.
   if (decl->isFromASTFile())
      return;

   // Iterate into extern "C" {} type declarations.
   if (auto linkageDecl = dyn_cast<clang::LinkageSpecDecl>(decl)) {
      for (auto *decl : linkageDecl->noload_decls()) {
         populateTableWithDecl(table, nameImporter, decl);
      }
      return;
   }

   // Skip non-named declarations.
   auto named = dyn_cast<clang::NamedDecl>(decl);
   if (!named)
      return;

   // Add this entry to the lookup table.
   addEntryToLookupTable(table, named, nameImporter);
}

void TypePHPLookupTableWriter::populateTable(TypePHPLookupTable &table,
                                             NameImporter &nameImporter) {
   auto &sema = nameImporter.getClangSema();
   for (auto decl : sema.Context.getTranslationUnitDecl()->noload_decls()) {
      populateTableWithDecl(table, nameImporter, decl);
   }

   // Add macros to the lookup table.
   addMacrosToLookupTable(table, nameImporter);

   // Finalize the lookup table, which may fail.
   finalizeLookupTable(table, nameImporter, buffersForDiagnostics);
};

std::unique_ptr<clang::ModuleFileExtensionWriter>
TypePHPNameLookupExtension::createExtensionWriter(clang::ASTWriter &writer) {
   return std::make_unique<TypePHPLookupTableWriter>(this, writer, typePHPCtx,
                                                      buffersForDiagnostics,
                                                      availability,
                                                      inferImportAsMember);
}

std::unique_ptr<clang::ModuleFileExtensionReader>
TypePHPNameLookupExtension::createExtensionReader(
   const clang::ModuleFileExtensionMetadata &metadata,
   clang::ASTReader &reader, clang::serialization::ModuleFile &mod,
   const llvm::BitstreamCursor &stream) {
   // Make sure we have a compatible block. Since these values are part
   // of the hash, it should never be wrong.
   assert(metadata.BlockName == "swift.lookup");
   assert(metadata.MajorVersion == POLAR_LOOKUP_TABLE_VERSION_MAJOR);
   assert(metadata.MinorVersion == POLAR_LOOKUP_TABLE_VERSION_MINOR);

   std::function<void()> onRemove = [](){};
   std::unique_ptr<TypePHPLookupTable> *target = nullptr;

   if (mod.Kind == clang::serialization::MK_PCH) {
      // PCH imports unconditionally overwrite the provided pchLookupTable.
      target = &pchLookupTable;
   } else {
      // Check whether we already have an entry in the set of lookup tables.
      target = &lookupTables[mod.ModuleName];
      if (*target) return nullptr;

      // Local function used to remove this entry when the reader goes away.
      std::string moduleName = mod.ModuleName;
      onRemove = [this, moduleName]() {
         lookupTables.erase(moduleName);
      };
   }

   // Create the reader.
   auto tableReader = TypePHPLookupTableReader::create(this, reader, mod, onRemove,
                                                       stream);
   if (!tableReader) return nullptr;

   // Create the lookup table.
   target->reset(new TypePHPLookupTable(tableReader.get()));

   // Return the new reader.
   return std::move(tableReader);
}
