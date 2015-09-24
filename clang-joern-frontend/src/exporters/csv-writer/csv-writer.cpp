#include "csv-writer.h"
#include "clang/AST/Expr.h"
#include <sys/stat.h>

using namespace clang;

namespace exporter {

bool csvFilesExist() {
  struct stat buf;
  std::string nodeFileName = "nodes.csv";
  std::string edgeFileName = "edges.csv";
  if ((stat(nodeFileName.c_str(), &buf) != -1) ||
      (stat(edgeFileName.c_str(), &buf) != -1))
      return true;
  return false;
}

void csvWriter::writeHeaders() {
  csvHeaderTy nodeHeader = {
      "nodeID:ID", "nodeKind", "loc", "locRange", \
      "type", "valueKind", "value", "castKind", \
      "declName"
  };

  csvHeaderTy edgeHeader = {
      "nodeID:ID", "nodeID:ID", "type"
  };

  // Write nodes.csv, edges.csv headers
  for (auto &i : nodeHeader) {
      if (i != nodeHeader.back())
        nodeFile << i << "\t";
      else
        nodeFile << i << std::endl;
  }

  for (auto &i : edgeHeader) {
      if (i != edgeHeader.back())
        edgeFile << i << "\t";
      else
        edgeFile << i << std::endl;
  }
}

void csvWriter::init() {
  if (!csvFilesExist()) {
    nodeFile.open("nodes.csv");
    edgeFile.open("edges.csv");
    writeHeaders();
  }
  else {
    nodeFile.open("nodes.csv", std::ofstream::app);
    edgeFile.open("edges.csv", std::ofstream::app);
  }
}

void csvWriter::flushBuffers() {
  nodeRowMap.clear();
  edgeRowMap.clear();
}

void csvWriter::writeNodeRowWrapper() {
  if (!nodeRowMap.empty())
    writeNodeRow(nodeRowMap, nodeFile);
}

void csvWriter::writeNodeRow(csvRowTy &row, fileTy &file) {
  for (int i = NODECOL::FIRST; i <= NODECOL::LAST; i++) {
      auto column = row.find(i);
      if (column == row.end()) {
	  if (i == NODECOL::LAST)
	    file << "" << std::endl;
	  else
	    file << "" << "\t";
	  continue;
      }
      if (i == NODECOL::LAST)
	file << column->second << std::endl;
      else
	file << column->second << "\t";
  }
}

void csvWriter::exportDecl(const Decl *D) {

  {
  nodeRowMap.emplace(NODECOL::NODEID, std::to_string(++nodeID));
  nodeRowMap.emplace(NODECOL::NODEKIND, D->getDeclKindName());
  nodeRowMap.emplace(NODECOL::LOC, printLocation(D->getLocation()));
  nodeRowMap.emplace(NODECOL::LOCRANGE, printSourceRange(D->getSourceRange()));
  }
}

void csvWriter::exportStmt(const Stmt *S) {

  {
  nodeRowMap.emplace(NODECOL::NODEID, std::to_string(++nodeID));
  nodeRowMap.emplace(NODECOL::NODEKIND, S->getStmtClassName());
  nodeRowMap.emplace(NODECOL::LOC, "");
  nodeRowMap.emplace(NODECOL::LOCRANGE, printSourceRange(S->getSourceRange()));
  }
}

void csvWriter::exportExpr(const Expr *E) {
  {
    switch (E->getValueKind()) {
    case VK_RValue:
      nodeRowMap.emplace(NODECOL::VALUEKIND, "");
      break;
    case VK_LValue:
      nodeRowMap.emplace(NODECOL::VALUEKIND, "lvalue");
      break;
    case VK_XValue:
      nodeRowMap.emplace(NODECOL::VALUEKIND, "xvalue");
      break;
    }
  }
}

void csvWriter::closeFiles() {
  nodeFile.close();
  edgeFile.close();
}

std::string csvWriter::printSourceRange(SourceRange SR) {

  std::string sourceRange;
  llvm::raw_string_ostream OS(sourceRange);

  OS << " <";
  OS << printLocation(SR.getBegin());
  if (SR.getBegin() != SR.getEnd()) {
    OS << ", ";
    OS << printLocation(SR.getEnd());
  }
  OS << ">";
  return OS.str();
}

std::string csvWriter::printLocation(SourceLocation SL) {
  std::string location;
  llvm::raw_string_ostream OS(location);

  SourceManager &SM = getASTContext()->getSourceManager();

  SourceLocation SpellingLoc = SM.getSpellingLoc(SL);

  // The general format we print out is filename:line:col, but we drop pieces
  // that haven't changed since the last loc printed.
  PresumedLoc PLoc = SM.getPresumedLoc(SpellingLoc);

  if (PLoc.isInvalid()) {
    OS << "<invalid sloc>";
    return OS.str();
  }

  if (strcmp(PLoc.getFilename(), lastLocFilename) != 0) {
    OS << PLoc.getFilename() << ':' << PLoc.getLine()
       << ':' << PLoc.getColumn();
    lastLocFilename = PLoc.getFilename();
    lastLocLineNum = PLoc.getLine();
  } else if (PLoc.getLine() != lastLocLineNum) {
    OS << "line" << ':' << PLoc.getLine()
       << ':' << PLoc.getColumn();
    lastLocLineNum = PLoc.getLine();
  } else {
    OS << "col" << ':' << PLoc.getColumn();
  }

  return OS.str();
}

} // end of exporter namespace
