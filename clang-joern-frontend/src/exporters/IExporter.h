#ifndef CJ_IEXPORTER_H
#define CJ_IEXPORTER_H

#include "clang/AST/ASTContext.h"

namespace exporter {

class IExporter {
public:
  IExporter(clang::ASTContext *ASTC) : Context(ASTC) {}
  virtual void exportDecl(const clang::Decl *D) = 0;
  virtual void exportNamedDecl(const clang::NamedDecl *ND) = 0;
  virtual void exportValueDecl(const clang::ValueDecl *VD) = 0;
  virtual void exportTranslationUnitDecl(std::string filename) = 0;
  virtual void exportStmt(const clang::Stmt *S) = 0;
  virtual void exportExpr(const clang::Expr *E) = 0;
  virtual void exportCastExpr(const clang::CastExpr *CE) = 0;
  virtual void exportDeclRefExpr(const clang::DeclRefExpr *DRE) = 0;
  virtual void exportCharacterLiteral(const clang::CharacterLiteral *CL) = 0;
  virtual void exportIntegerLiteral(const clang::IntegerLiteral *IL) = 0;
  virtual void exportFloatingLiteral(const clang::FloatingLiteral *FL) = 0;
  virtual void exportStringLiteral(const clang::StringLiteral *SL) = 0;
  virtual ~IExporter() {}
  clang::ASTContext *getASTContext() { return Context; }
private:
  clang::ASTContext *Context;
};

} // end of exporter namespace

#endif // CJ_IEXPORTER_H
