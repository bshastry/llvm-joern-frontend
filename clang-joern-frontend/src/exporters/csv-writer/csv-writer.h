#ifndef CJ_CSV_WRITER_H
#define CJ_CSV_WRITER_H

#include <fstream>
#include "IExporter.h"

namespace exporter {

class csvWriter : public IExporter {
public:
	void init();
	void closeFiles();
	csvWriter() { init(); }
	virtual ~csvWriter() { closeFiles(); }
	void writeNodeHeader();
	void writeEdgeHeader();
	void exportDecl(const clang::Decl *D) override;
	void exportStmt(const clang::Stmt *S) override;

	std::ofstream nodeFile, edgeFile;
};

} // end of exporter namespace

#endif // CJ_CSV_WRITER_H
