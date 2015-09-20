#ifndef CJ_IEXPORTER_H
#define CJ_IEXPORTER_H

#include "clang/AST/ASTContext.h"

using namespace clang;

namespace exporter {

class IExporter {
public:
  virtual void exportDecl(const Decl *D) = 0;
  virtual void exportStmt(const Stmt *S) = 0;
  virtual ~IExporter() {}
};

} // end of exporter namespace

#endif // CJ_IEXPORTER_H
