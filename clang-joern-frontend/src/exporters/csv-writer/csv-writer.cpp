#include "csv-writer.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Decl.h"
#include <sys/stat.h>

using namespace clang;
using namespace llvm;

namespace exporter {

using codePropTy	 = csvWriter::codePropTy;
using nodeIDTy 		 = csvWriter::nodeIDTy;
using fileTy 		 = csvWriter::fileTy;

bool csvFilesExist() {
  struct stat buf;
  std::string nodeFileName = "nodes.csv";
  std::string edgeFileName = "edges.csv";
  if ((stat(nodeFileName.c_str(), &buf) != -1) ||
      (stat(edgeFileName.c_str(), &buf) != -1))
      return true;
  return false;
}

bool doesNodeIDFileExist() {
  struct stat buf;
  std::string nodeIDFilename = ".nodeID";
  if (stat(nodeIDFilename.c_str(), &buf) != -1)
    return true;
  return false;
}

nodeIDTy readPrevNodeIDFromFile() {
  std::fstream nodeFile;
  nodeIDTy tmp;
  nodeFile.open(".nodeID", std::ios::in);
  nodeFile >> tmp;
  // Delete file
  if (remove(".nodeID") != 0)
    llvm_unreachable("Error deleting nodeID file");
  return tmp;
}

void writeNodeIDToTmpFile(nodeIDTy id) {
  fileTy nodeIDFile;
  nodeIDFile.open(".nodeID", std::ios::out);
  nodeIDFile << id;
  nodeIDFile.close();
}

void csvWriter::writeHeaders() {
  csvHeaderTy nodeHeader = {
      "nodeID:ID", "nodeKind", "loc", "locRange", \
      "type", "valueKind", "value", "castKind", \
      "declName", "semcontext", "lexcontext", \
      "declqual", "baredeclref"
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
    nodeID = readPrevNodeIDFromFile();
  }
}

void csvWriter::flushBuffers() {
  nodeRowMap.clear();
  edgeRowMap.clear();
}

void csvWriter::writeNodeRowWrapper() {
  if (!nodeRowMap.empty())
    writeRow(nodeRowMap, nodeFile, FIRST, LAST);
}

void csvWriter::writeEdgeRowWrapper() {
  if (!edgeRowMap.empty())
    writeRow(edgeRowMap, edgeFile, EFIRST, ELAST);
}

void csvWriter::writeRow(csvRowTy &row, fileTy &file, colIndexTy start, colIndexTy end) {
  for (colIndexTy i = start; i <= end; i++) {
      auto column = row.find(i);
      if (column == row.end()) {
	  if (i == end)
	    file << "" << std::endl;
	  else
	    file << "" << "\t";
	  continue;
      }
      if (i == end)
	file << column->second << std::endl;
      else
	file << column->second << "\t";
  }
}

void csvWriter::writeEdgeRow(codePropTy node1, codePropTy node2, codePropTy rel) {
  edgeRowMap.emplace(ENODEID1, node1);
  edgeRowMap.emplace(ENODEID2, node2);
  edgeRowMap.emplace(ETYPE, rel);
  writeRow(edgeRowMap, edgeFile, EFIRST, ELAST);
  edgeRowMap.clear();
}

void csvWriter::exportDecl(const Decl *D) {

  declNodeMap.emplace(D, ++nodeID);
  {
    if (D->getLexicalDeclContext() != D->getDeclContext())
      writeEdgeRow(std::to_string(nodeID),
                   getNodeIDFromDeclPtr(cast<Decl>(D->getDeclContext())),
                   EDGERELKEYS[SEMANTIC_PARENT]);

    nodeRowMap.emplace(NODEID, std::to_string(nodeID));
    nodeRowMap.emplace(NODEKIND, D->getDeclKindName());
    // Defer filling location to TUD visit
    if (!isa<TranslationUnitDecl>(D))
      nodeRowMap.emplace(LOC, getLocation(D->getLocation()));
    nodeRowMap.emplace(LOCRANGE, getSourceRange(D->getSourceRange()));
  }
}

void csvWriter::exportNamedDecl(const NamedDecl *ND) {
    if (ND->getDeclName())
      nodeRowMap.emplace(DECLNAME, ND->getNameAsString());
}

void csvWriter::exportTranslationUnitDecl(std::string filename) {
  nodeRowMap.emplace(LOC, filename);
}

void csvWriter::exportStmt(const Stmt *S) {

  stmtNodeMap.emplace(S, ++nodeID);
  {
    nodeRowMap.emplace(NODEID, std::to_string(nodeID));
    nodeRowMap.emplace(NODEKIND, S->getStmtClassName());
    nodeRowMap.emplace(LOC, "");
    nodeRowMap.emplace(LOCRANGE, getSourceRange(S->getSourceRange()));
  }
}

void csvWriter::exportExpr(const Expr *E) {
  nodeRowMap.emplace(TYPE, getType(E->getType()));
  {
    switch (E->getValueKind()) {
    case VK_RValue:
      nodeRowMap.emplace(VALUEKIND, "");
      break;
    case VK_LValue:
      nodeRowMap.emplace(VALUEKIND, "lvalue");
      break;
    case VK_XValue:
      nodeRowMap.emplace(VALUEKIND, "xvalue");
      break;
    }
  }
}

void csvWriter::exportCastExpr(const CastExpr *CE) {
  nodeRowMap.emplace(CASTKIND, CE->getCastKindName());
}

void csvWriter::exportDeclRefExpr(const DeclRefExpr *DRE) {
  codePropTy declNode = getNodeIDFromDeclPtr(llvm::cast<Decl>(DRE->getDecl()));

  // Only in rare cases is a decl that is reference is not yet visited by RAV
  // In such cases, we export declref info to baredeclref column of the node
  // that refs the decl i.e., referee decl info is dumped in referer's node row.
  if (declNode.empty())
    nodeRowMap.emplace(BAREDECLREF, getBareDeclRef(llvm::cast<Decl>(DRE->getDecl())));
  else {
    edgeRowMap.emplace(ENODEID1, std::to_string(nodeID));
    edgeRowMap.emplace(ENODEID2, declNode);
    edgeRowMap.emplace(ETYPE, EDGERELKEYS[DECLREF_EXPR]);
    writeRow(edgeRowMap, edgeFile, EFIRST, ELAST);
    edgeRowMap.clear();
  }
}

void csvWriter::closeFiles() {
  nodeFile.close();
  edgeFile.close();
  writeNodeIDToTmpFile(nodeID);
}

codePropTy csvWriter::getSourceRange(SourceRange SR) {

  codePropTy sourceRange;
  llvm::raw_string_ostream OS(sourceRange);

  OS << " <";
  OS << getLocation(SR.getBegin());
  if (SR.getBegin() != SR.getEnd()) {
    OS << ", ";
    OS << getLocation(SR.getEnd());
  }
  OS << ">";
  return OS.str();
}

codePropTy csvWriter::getLocation(SourceLocation SL) {
  codePropTy location;
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

// Obtain both sugared and optionally desugared types
codePropTy csvWriter::getBareType(QualType T, bool Desugar) {
  codePropTy typeinfo;
  llvm::raw_string_ostream OS(typeinfo);

  SplitQualType T_split = T.split();
  OS << "<" << QualType::getAsString(T_split);

  if (Desugar && !T.isNull()) {
    // If the type is sugared, also dump a (shallow) desugared type.
    SplitQualType D_split = T.getSplitDesugaredType();
    if (T_split != D_split)
      OS << ", " << QualType::getAsString(D_split) << ">";
    else
      OS << ">";
  }
  else {
    OS << ">";
  }
  return OS.str();
}

codePropTy csvWriter::getType(QualType T) {
  return getBareType(T);
}

codePropTy csvWriter::getNodeIDFromDeclPtr(const Decl *D) {
  auto i = declNodeMap.find(D);
  if (i != declNodeMap.end())
    return std::to_string(i->second);
  return codePropTy();
}

codePropTy csvWriter::getBareDeclRef(const Decl *D) {
  codePropTy declRefInfo;
  llvm::raw_string_ostream OS(declRefInfo);

  OS << "<";
  OS << D->getDeclKindName();

  if (const NamedDecl *ND = dyn_cast<NamedDecl>(D)) {
    OS << ", " << ND->getDeclName();
  }

  if (const ValueDecl *VD = dyn_cast<ValueDecl>(D))
    OS << ", " << getType(VD->getType());

  OS << ">";

  return OS.str();
}

codePropTy csvWriter::getNodeIDFromStmtPtr(const Stmt *S) {
  auto i = stmtNodeMap.find(S);
  if (i != stmtNodeMap.end())
    return std::to_string(i->second);
  return codePropTy();
}

} // end of exporter namespace
