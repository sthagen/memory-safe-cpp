/* -------------------------------------------------------------------------------
* Copyright (c) 2020, OLogN Technologies AG
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the OLogN Technologies AG nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* -------------------------------------------------------------------------------*/

#ifndef NODECPP_CHECKER_RULEM1_H
#define NODECPP_CHECKER_RULEM1_H


#include "nodecpp/NakedPtrHelper.h"
#include "ClangTidyDiagnosticConsumer.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTLambda.h"

namespace nodecpp {
namespace checker {


class RuleM1ASTVisitor
  : public clang::RecursiveASTVisitor<RuleM1ASTVisitor> {

  typedef clang::RecursiveASTVisitor<RuleM1ASTVisitor> Super;

  ClangTidyContext &Context;
//  MyStack St;


  /// \brief Add a diagnostic with the check's name.
  DiagnosticBuilder diag(SourceLocation Loc, StringRef Message,
                         DiagnosticIDs::Level Level = DiagnosticIDs::Error) {
    return Context.diag(DiagMsgSrc, Loc, Message, Level);
  }

  DiagHelper makeDiagHelper() {
    return DiagHelper(std::bind(&RuleM1ASTVisitor::diag, this, std::placeholders::_1,
    std::placeholders::_2, std::placeholders::_3), DiagnosticIDs::Note);
  }

public:

  explicit RuleM1ASTVisitor(ClangTidyContext &Context): Context(Context) {}

  bool TraverseDecl(Decl *D) {
    //mb: we don't traverse decls in system-headers
    //TranslationUnitDecl has an invalid location, but needs traversing anyway

    if(!D)
      return true;

    else if (isa<TranslationUnitDecl>(D))
      return Super::TraverseDecl(D);

    else if(isSystemLocation(&Context, D->getLocation()))
        return true;

    else
      return Super::TraverseDecl(D);
  }

  bool VisitCXXThrowExpr(clang::CXXThrowExpr *Expr) {
    
    auto Qt = Expr->getSubExpr()->getType();
    if(!isNodecppErrorType(Qt))
      diag(Expr->getExprLoc(), "(M1.1) throw expression must be of nodecpp::error type");

    return Super::VisitCXXThrowExpr(Expr);
  }


  bool VisitCXXCatchStmt(clang::CXXCatchStmt *Stmt) {
    
    auto Qt = Stmt->getCaughtType();
    if(!Qt->isLValueReferenceType()) {
      diag(Stmt->getExceptionDecl()->getLocation(), "(M1.2) catch type must be reference to nodecpp::error type");
    }
    else {
      auto Qt2 = Qt->getPointeeType();
      if(!isNodecppErrorType(Qt2)) {
        diag(Stmt->getExceptionDecl()->getLocation(), "(M1.2) catch type must be reference to nodecpp::error type");
      }
    }

    return Super::VisitCXXCatchStmt(Stmt);
  }

};

class RuleM1ASTConsumer : public clang::ASTConsumer {

  RuleM1ASTVisitor Visitor;

public:
  RuleM1ASTConsumer(ClangTidyContext &Context) :Visitor(Context) {}

  void HandleTranslationUnit(clang::ASTContext &Context) override {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

};

} // namespace checker
} // namespace nodecpp

#endif // NODECPP_CHECKER_RULEM1_H

