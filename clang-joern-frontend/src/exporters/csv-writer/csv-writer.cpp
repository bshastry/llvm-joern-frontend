#include "csv-writer.h"
#include <sys/stat.h>

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

void csvWriter::init() {
  if (!csvFilesExist()) {
    nodeFile.open("nodes.csv", std::ofstream::app);
    edgeFile.open("edges.csv", std::ofstream::app);
    writeNodeHeader();
    writeEdgeHeader();
  }
  else {
    nodeFile.open("nodes.csv", std::ofstream::app);
    edgeFile.open("edges.csv", std::ofstream::app);
  }
}

void csvWriter::exportDecl(const Decl *D) {
  nodeFile << D->getDeclKindName() << "\n";
  edgeFile << "Rubbish" << "\n";
}

void csvWriter::exportStmt(const Stmt *S) {

}

void csvWriter::closeFiles() {
  nodeFile.close();
  edgeFile.close();
}

void csvWriter::writeNodeHeader() {
  nodeFile << "nodeDesc" << "\n";
}

void csvWriter::writeEdgeHeader() {
  edgeFile << "edgeDesc" << "\n";
}

} // end of exporter namespace
