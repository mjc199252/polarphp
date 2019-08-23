// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/08.

#ifndef POLARPHP_SYNTAX_SYNTAX_NODE_STMT_NODES_H
#define POLARPHP_SYNTAX_SYNTAX_NODE_STMT_NODES_H

#include "polarphp/syntax/Syntax.h"
#include "polarphp/syntax/TokenSyntax.h"
#include "polarphp/syntax/UnknownSyntax.h"
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/StmtSyntaxNodesFwd.h"

namespace polar::syntax {

///
/// empty_stmt:
///   ';'
///
class EmptyStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_SEMICOLON)
      /// optional: false
      ///
      Semicolon,
   };

public:
   EmptyStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getSemicolon();
   EmptyStmtSyntax withSemicolon(std::optional<TokenSyntax> semicolon);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::EmptyStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class EmptyStmtSyntaxBuilder;
   void validate();
};

///
///  condition -> expression
///
class ConditionElementSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: Syntax
      /// optional: false
      /// ------------
      /// node choices
      /// name: Expr kind: ExprSyntax
      Condition,
      /// type: TokenSyntax
      /// optional: true
      TrailingComma,
   };

#ifdef POLAR_DEBUG_BUILD
   ///
   /// child name: Condition
   /// choices:
   /// SyntaxKind::Expr
   ///
   static const NodeChoicesType CHILD_NODE_CHOICES;
#endif
public:
   ConditionElementSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   Syntax getCondition();
   std::optional<TokenSyntax> getTrailingComma();

   ConditionElementSyntax withCondition(std::optional<Syntax> condition);
   ConditionElementSyntax withTrailingComma(std::optional<TokenSyntax> trailingComma);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ConditionElement;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ConditionElementSyntaxBuilder;
   void validate();
};

///
/// continue_stmt:
///   T_CONTINUE optional_expr ';'
///
class ContinueStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;

   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      ContinueKeyword,
      ///
      /// type: ExprSyntax
      /// optional: true
      ///
      Expr,
   };
public:
   ContinueStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getContinueKeyword();
   std::optional<ExprSyntax> getExpr();

   ContinueStmtSyntax withContinueKeyword(std::optional<TokenSyntax> continueKeyword);
   ContinueStmtSyntax withExpr(std::optional<ExprSyntax> expr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ContinueStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class ContinueStmtSyntaxBuilder;
   void validate();
};

class BreakStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;

   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      BreakKeyword,
      ///
      /// type: TokenSyntax
      /// optional: true
      ///
      LNumberToken,
   };
public:
   BreakStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getBreakKeyword();
   std::optional<TokenSyntax> getLNumberToken();

   BreakStmtSyntax withBreakKeyword(std::optional<TokenSyntax> breakKeyword);
   BreakStmtSyntax withLNumberToken(std::optional<TokenSyntax> numberToken);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::BreakStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class BreakStmtSyntaxBuilder;
   void validate();
};

class FallthroughStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;

   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_FALLTHROUGH)
      /// optional: false
      ///
      FallthroughKeyword
   };

public:
   FallthroughStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getFallthroughKeyword();
   FallthroughStmtSyntax withFallthroughStmtSyntax(std::optional<TokenSyntax> fallthroughKeyword);

private:
   friend class FallthroughStmtSyntaxBuilder;
   void validate();
};

///
/// if_stmt_without_else:
///   T_IF '(' expr ')' statement
/// | if_stmt_without_else T_ELSEIF '(' expr ')' statement
///
class ElseIfClauseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 5;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 5;

   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_ELSEIF)
      /// optional: false
      ///
      ElseIfKeyword,
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParen,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Condition,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParen,
      ///
      /// type: CodeBlockSyntax
      /// optional: false
      ///
      Body
   };

public:
   ElseIfClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getElseIfKeyword();
   TokenSyntax getLeftParen();
   ExprSyntax getCondition();
   TokenSyntax getRightParen();
   CodeBlockSyntax getBody();

   ElseIfClauseSyntax withElseIfKeyword(std::optional<TokenSyntax> elseIfKeyword);
   ElseIfClauseSyntax withLeftParen(std::optional<TokenSyntax> leftParen);
   ElseIfClauseSyntax withCondition(std::optional<ExprSyntax> condition);
   ElseIfClauseSyntax withRightParen(std::optional<TokenSyntax> rightParen);
   ElseIfClauseSyntax withBody(std::optional<CodeBlockSyntax> body);

private:
   friend class ElseIfClauseSyntaxBuilder;
   void validate();
};

///
/// if_stmt:
///   if_stmt_without_else %prec T_NOELSE
/// | if_stmt_without_else T_ELSE statement
///
class IfStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 10;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 5;

   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_IDENTIFIER_STRING)
      /// optional: true
      ///
      LabelName,
      ///
      /// type: TokenSyntax (T_COLON)
      /// optional: true
      ///
      LabelColon,
      ///
      /// type: TokenSyntax (T_IF)
      /// optional: false
      ///
      IfKeyword,
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParen,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Condition,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParen,
      ///
      /// type: CodeBlockSyntax
      /// optional: false
      ///
      Body,
      ///
      /// type: ElseIfListSyntax
      /// is_syntax_collection: true
      /// optional: true
      ///
      ElseIfClauses,
      ///
      /// type: TokenSyntax (T_ELSE)
      /// optional: true
      ///
      ElseKeyword,
      ///
      /// type: Syntax
      /// optional: true
      /// --------------
      /// node choices
      /// name: IfStmt kind: IfStmt
      /// name: CodeBlock kind: CodeBlock
      ///
      ElseBody
   };

#ifdef POLAR_DEBUG_BUILD
   static const NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   IfStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getLabelName();
   std::optional<TokenSyntax> getLabelColon();
   TokenSyntax getIfKeyword();
   TokenSyntax getLeftParen();
   ExprSyntax getCondition();
   TokenSyntax getRightParen();
   CodeBlockSyntax getBody();
   std::optional<ElseIfListSyntax> getElseIfClauses();
   std::optional<TokenSyntax> getElseKeyword();
   std::optional<Syntax> getElseBody();

   IfStmtSyntax withLabelName(std::optional<TokenSyntax> labelName);
   IfStmtSyntax withLabelColon(std::optional<TokenSyntax> labelColon);
   IfStmtSyntax withIfKeyword(std::optional<TokenSyntax> ifKeyword);
   IfStmtSyntax withLeftParen(std::optional<TokenSyntax> leftParen);
   IfStmtSyntax withCondition(std::optional<ExprSyntax> condition);
   IfStmtSyntax withRightParen(std::optional<TokenSyntax> rightParen);
   IfStmtSyntax withBody(std::optional<CodeBlockSyntax> body);
   IfStmtSyntax withElseIfClauses(std::optional<ElseIfListSyntax> elseIfClauses);
   IfStmtSyntax withElseKeyword(std::optional<TokenSyntax> elseKeyword);
   IfStmtSyntax withElseBody(std::optional<Syntax> elseBody);

   IfStmtSyntax addElseIfClause(ElseIfClauseSyntax elseIfClause);
private:
   friend class IfStmtSyntaxBuilder;
   void validate();
};

///
/// while_stmt:
///   T_WHILE '(' expr ')' while_statement
///
class WhileStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 7;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 5;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_IDENTIFIER_STRING)
      /// optional: true
      ///
      LabelName,
      ///
      /// type: TokenSyntax (T_COLON)
      /// optional: true
      ///
      LabelColon,
      ///
      /// type: TokenSyntax (T_WHILE)
      /// optional: false
      ///
      WhileKeyword,
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParen,
      ///
      /// type: ConditionElementListSyntax
      /// optional: false
      ///
      Conditions,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParen,
      ///
      /// type: CodeBlockSyntax
      /// optional: false
      ///
      Body
   };

public:
   WhileStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getLabelName();
   std::optional<TokenSyntax> getLabelColon();
   TokenSyntax getWhileKeyword();
   TokenSyntax getLeftParen();
   ConditionElementListSyntax getConditions();
   TokenSyntax getRightParen();
   CodeBlockSyntax getBody();

   WhileStmtSyntax withLabelName(std::optional<TokenSyntax> labelName);
   WhileStmtSyntax withLabelColon(std::optional<TokenSyntax> labelColon);
   WhileStmtSyntax withWhileKeyword(std::optional<TokenSyntax> whileKeyword);
   WhileStmtSyntax withLeftParen(std::optional<TokenSyntax> leftParen);
   WhileStmtSyntax withConditions(std::optional<ConditionElementListSyntax> conditions);
   WhileStmtSyntax withRightParen(std::optional<TokenSyntax> rightParen);
   WhileStmtSyntax withBody(std::optional<CodeBlockSyntax> body);

   /// Adds the provided `condition` to the node's `Conditions`
   /// collection.
   /// - param element: The new `condition` to add to the node's
   ///                  `Conditions` collection.
   /// - returns: A copy of the receiver with the provided `condition`
   ///            appended to its `Conditions` collection.
   WhileStmtSyntax addCondition(ConditionElementSyntax condition);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::WhileStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class WhileStmtSyntaxBuilder;
   void validate();
};

class DoWhileStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 8;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 6;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_IDENTIFIER_STRING)
      /// optional: true
      ///
      LabelName,
      ///
      /// type: TokenSyntax (T_COLON)
      /// optional: true
      ///
      LabelColon,
      ///
      /// type: TokenSyntax (T_DO)
      /// optional: false
      ///
      DoKeyword,
      ///
      /// type: CodeBlockSyntax
      /// optional: false
      ///
      Body,
      ///
      /// type: TokenSyntax (T_WHILE)
      /// optional: false
      ///
      WhileKeyword,
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParen,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Condition,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParen
   };

public:
   DoWhileStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getLabelName();
   std::optional<TokenSyntax> getLabelColon();
   TokenSyntax getDoKeyword();
   CodeBlockSyntax getBody();
   TokenSyntax getWhileKeyword();
   TokenSyntax getLeftParen();
   ExprSyntax getCondition();
   TokenSyntax getRightParen();

   DoWhileStmtSyntax withLabelName(std::optional<TokenSyntax> labelName);
   DoWhileStmtSyntax withLabelColon(std::optional<TokenSyntax> labelColon);
   DoWhileStmtSyntax withDoKeyword(std::optional<TokenSyntax> doKeyword);
   DoWhileStmtSyntax withBody(std::optional<CodeBlockSyntax> body);
   DoWhileStmtSyntax withWhileKeyword(std::optional<TokenSyntax> whileKeyword);
   DoWhileStmtSyntax withLeftParen(std::optional<TokenSyntax> leftParen);
   DoWhileStmtSyntax withCondition(std::optional<ExprSyntax> condition);
   DoWhileStmtSyntax withRightParen(std::optional<TokenSyntax> rightParen);

private:
   friend class DoWhileStmtSyntaxBuilder;
   void validate();
};

///
/// \brief The SwitchDefaultLabelSyntax class
///
/// switch-default-label -> 'default' ':'
///
class SwitchDefaultLabelSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_DEFAULT)
      /// optional: false
      ///
      DefaultKeyword,
      ///
      /// type: TokenSyntax (T_COLON)
      /// optional: false
      ///
      Colon,
   };

public:
   SwitchDefaultLabelSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getDefaultKeyword();
   TokenSyntax getColon();

   SwitchDefaultLabelSyntax withDefaultKeyword(std::optional<TokenSyntax> defaultKeyword);
   SwitchDefaultLabelSyntax withColon(std::optional<TokenSyntax> colon);

private:
   friend class SwitchDefaultLabelSyntaxBuilder;
   void validate();
};

///
/// \brief The SwitchCaseLabelSyntax class
///
/// switch-case-label -> 'case' case-item-list ':'
///
class SwitchCaseLabelSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_CASE)
      /// optional: false
      ///
      CaseKeyword,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Expr,
      ///
      /// type: TokenSyntax (T_COLON)
      /// optional: false
      ///
      Colon
   };
public:
   SwitchCaseLabelSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getCaseKeyword();
   ExprSyntax getExpr();
   TokenSyntax getColon();

   SwitchCaseLabelSyntax withCaseKeyword(std::optional<TokenSyntax> caseKeyword);
   SwitchCaseLabelSyntax withExpr(std::optional<ExprSyntax> expr);
   SwitchCaseLabelSyntax withColon(std::optional<TokenSyntax> colon);

private:
   friend class SwitchCaseLabelSyntaxBuilder;
   void validate();
};

///
/// \brief The SwitchCaseSyntax class
///
/// switch-case -> switch-case-label stmt-list
///              | switch-default-label stmt-list
///
class SwitchCaseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: Syntax
      /// optional: false
      /// node choices
      /// -------------------------------------------
      /// node choice: SwitchDefaultLabelSyntax
      /// -------------------------------------------
      /// node choice: SwitchCaseLabelSyntax
      ///
      Label,
      ///
      /// type: CodeBlockItemListSyntax
      /// optional: false
      ///
      Statements
   };

#ifdef POLAR_DEBUG_BUILD
   static const NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   SwitchCaseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   Syntax getLabel();
   CodeBlockItemListSyntax getStatements();

   SwitchCaseSyntax withLabel(std::optional<Syntax> label);
   SwitchCaseSyntax withStatements(std::optional<CodeBlockItemListSyntax> statements);
   SwitchCaseSyntax addStatement(CodeBlockItemSyntax statement);

private:
   friend class SwitchCaseSyntaxBuilder;
   void validate();
};

///
/// \brief The SwitchStmtSyntax class
///
/// switch-stmt -> identifier? ':'? 'switch' '(' expr ')' '{'
///    switch-case-list '}'
///
class SwitchStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 9;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 7;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_IDENTIFIER_STRING)
      /// optional: true
      ///
      LabelName,
      ///
      /// type: TokenSyntax (T_COLON)
      /// optional: true
      ///
      LabelColon,
      ///
      /// type: TokenSyntax (T_SWITCH)
      /// optional: false
      ///
      SwitchKeyword,
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParen,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      ConditionExpr,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParen,
      ///
      /// type: TokenSyntax (T_LEFT_BRACE)
      /// optional: false
      ///
      LeftBrace,
      ///
      /// is_syntax_collection: true
      /// type: SwitchCaseListSyntax
      /// optional: false
      ///
      Cases,
      ///
      /// type: TokenSyntax (T_RIGHT_BRACE)
      /// optional: false
      ///
      RightBrace
   };

public:
   SwitchStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getLabelName();
   std::optional<TokenSyntax> getLabelColon();
   TokenSyntax getSwitchKeyword();
   TokenSyntax getLeftParen();
   ExprSyntax getConditionExpr();
   TokenSyntax getRightParen();
   TokenSyntax getLeftBrace();
   SwitchCaseListSyntax getCases();
   TokenSyntax getRightBrace();

   SwitchStmtSyntax withLabelName(std::optional<TokenSyntax> labelName);
   SwitchStmtSyntax withLabelColon(std::optional<TokenSyntax> labelColon);
   SwitchStmtSyntax withSwitchKeyword(std::optional<TokenSyntax> switchKeyword);
   SwitchStmtSyntax withLeftParen(std::optional<TokenSyntax> leftParen);
   SwitchStmtSyntax withConditionExpr(std::optional<ExprSyntax> conditionExpr);
   SwitchStmtSyntax withRightParen(std::optional<TokenSyntax> rightParen);
   SwitchStmtSyntax withLeftBrace(std::optional<TokenSyntax> leftBrace);
   SwitchStmtSyntax withCases(std::optional<SwitchCaseListSyntax> cases);
   SwitchStmtSyntax withRightBrace(std::optional<TokenSyntax> rightBrace);

   SwitchStmtSyntax addCase(SwitchCaseSyntax switchCase);

private:
   friend class SwitchStmtSyntaxBuilder;
   void validate();
};

///
/// \brief The DeferStmtSyntax class
///
/// defer-stmt -> 'defer' code-block
///
class DeferStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_DEFER)
      /// optional: false
      ///
      DeferKeyword,
      ///
      /// type: CodeBlockSyntax
      /// optional: false
      ///
      Body
   };

public:
   DeferStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getDeferKeyword();
   CodeBlockSyntax getBody();
   DeferStmtSyntax withDeferKeyword(std::optional<TokenSyntax> deferKeyword);
   DeferStmtSyntax withBody(std::optional<CodeBlockSyntax> body);

private:
   friend class DeferStmtSyntaxBuilder;
   void validate();
};

///
/// \brief The ExpressionStmtSyntax class
///
/// expr-stmt -> expression ';'
///
class ExpressionStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Expr
   };
public:
   ExpressionStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   ExprSyntax getExpr();
   ExpressionStmtSyntax withExpr(std::optional<ExprSyntax> expr);

private:
   friend class ExpressionStmtSyntaxBuilder;
   void validate();
};

///
/// throw_stmt:
///   T_THROW expr ';'
///
class ThrowStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;

   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_THROW)
      /// optional: false
      ///
      ThrowKeyword,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Expr,
      ///
      /// type: TokenSyntax (T_SEMICLON)
      /// optioal: false
      ///
      Semicolon,
   };
public:
   ThrowStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getThrowKeyword();
   ExprSyntax getExpr();
   TokenSyntax getSemicolon();

   ThrowStmtSyntax withThrowKeyword(std::optional<TokenSyntax> throwKeyword);
   ThrowStmtSyntax withExpr(std::optional<ExprSyntax> expr);
   ThrowStmtSyntax withSemicolon(std::optional<TokenSyntax> semicolon);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ThrowStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ThrowStmtSyntaxBuilder;
   void validate();
};

///
/// return_stmt:
///   T_RETURN optional_expr ';'
///
class ReturnStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;

   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      ReturnKeyword,
      ///
      /// type: ExprSyntax
      /// optional: true
      ///
      Expr,
      ///
      /// type: TokenSyntax (T_SEMICLON)
      /// optioal: false
      ///
      Semicolon,
   };
public:
   ReturnStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getReturnKeyword();
   ExprSyntax getExpr();
   TokenSyntax getSemicolon();

   ReturnStmtSyntax withReturnKeyword(std::optional<TokenSyntax> returnKeyword);
   ReturnStmtSyntax withExpr(std::optional<ExprSyntax> expr);
   ReturnStmtSyntax withSemicolon(std::optional<TokenSyntax> semicolon);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ReturnStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ReturnStmtSyntaxBuilder;
   void validate();
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_NODE_STMT_NODES_H
