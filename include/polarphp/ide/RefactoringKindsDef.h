#ifndef REFACTORING
#define REFACTORING(KIND, NAME, ID)
#endif

#ifndef SEMANTIC_REFACTORING
#define SEMANTIC_REFACTORING(KIND, NAME, ID) REFACTORING(KIND, NAME, ID)
#endif

#ifndef RANGE_REFACTORING
#define RANGE_REFACTORING(KIND, NAME, ID) SEMANTIC_REFACTORING(KIND, NAME, ID)
#endif

#ifndef INTERNAL_RANGE_REFACTORING
#define INTERNAL_RANGE_REFACTORING(KIND, NAME, ID) RANGE_REFACTORING(KIND, NAME, ID)
#endif

#ifndef CURSOR_REFACTORING
#define CURSOR_REFACTORING(KIND, NAME, ID) SEMANTIC_REFACTORING(KIND, NAME, ID)
#endif

/// Rename and categorise the symbol occurrences at provided locations
/// (syntactically).
REFACTORING(GlobalRename, "Global Rename", rename.global)

/// Categorize source ranges for symbol occurrences at provided locations
/// (syntactically).
REFACTORING(FindGlobalRenameRanges, "Find Global Rename Ranges", rename.global.find-ranges)

/// Find and categorize all occurences of the file-local symbol at a given
/// location.
REFACTORING(FindLocalRenameRanges, "Find Local Rename Ranges", rename.local.find-ranges)

/// Find and rename all occurences of the file-local symbol at a given
/// location.
CURSOR_REFACTORING(LocalRename, "Local Rename", rename.local)

CURSOR_REFACTORING(FillInterfaceStub, "Add Missing Interface Requirements", fillstub)

CURSOR_REFACTORING(ExpandDefault, "Expand Default", expand.default)

CURSOR_REFACTORING(ExpandSwitchCases, "Expand Switch Cases", expand.switch.cases)

CURSOR_REFACTORING(LocalizeString, "Localize String", localize.string)

CURSOR_REFACTORING(SimplifyNumberLiteral, "Simplify Long Number Literal", simplify.long.number.literal)

CURSOR_REFACTORING(CollapseNestedIfStmt, "Collapse Nested If Statements", collapse.nested.ifstmt)

CURSOR_REFACTORING(ConvertToDoCatch, "Convert To Do/Catch", convert.do.catch)

CURSOR_REFACTORING(TrailingClosure, "Convert To Trailing Closure", trailingclosure)

CURSOR_REFACTORING(MemberwiseInitLocalRefactoring, "Generate Memberwise Initializer", memberwise.init.local.refactoring)

RANGE_REFACTORING(ExtractExpr, "Extract Expression", extract.expr)

RANGE_REFACTORING(ExtractFunction, "Extract Method", extract.function)

RANGE_REFACTORING(ExtractRepeatedExpr, "Extract Repeated Expression", extract.expr.repeated)

RANGE_REFACTORING(MoveMembersToExtension, "Move To Extension", move.members.to.extension)

RANGE_REFACTORING(ConvertStringsConcatenationToInterpolation, "Convert to String Interpolation", convert.string-concatenation.interpolation)

RANGE_REFACTORING(ExpandTernaryExpr, "Expand Ternary Expression", expand.ternary.expr)

RANGE_REFACTORING(ConvertToTernaryExpr, "Convert To Ternary Expression", convert.ternary.expr)

RANGE_REFACTORING(ConvertIfLetExprToGuardExpr, "Convert To Guard Expression", convert.iflet.to.guard.expr)

RANGE_REFACTORING(ConvertGuardExprToIfLetExpr, "Convert To IfLet Expression", convert.to.iflet.expr)

// These internal refactorings are designed to be helpful for working on
// the compiler/standard library, etc., but are likely to be just confusing and
// noise for general development.

INTERNAL_RANGE_REFACTORING(ReplaceBodiesWithFatalError, "Replace Function Bodies With 'fatalError()'", replace.bodies.with.fatalError)

#undef CURSOR_REFACTORING
#undef INTERNAL_RANGE_REFACTORING
#undef RANGE_REFACTORING
#undef SEMANTIC_REFACTORING
#undef REFACTORING
