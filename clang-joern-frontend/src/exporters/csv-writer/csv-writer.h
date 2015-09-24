#ifndef CJ_CSV_WRITER_H
#define CJ_CSV_WRITER_H

#include <fstream>
#include "IExporter.h"

namespace exporter {

class csvWriter : public IExporter {
public:
	// Typedefs
	enum NODECOL : int {
	  NODEID	=	0,
	  NODEKIND	=	1,
	  LOC		=	2,
	  LOCRANGE	=	3,
	  TYPE		=	4,
	  VALUEKIND	=	5,
	  VALUE		=	6,
	  CASTKIND	=	7,
	  DECLNAME	=	8,
	  FIRST		=	NODEID,
	  LAST		=	DECLNAME
	};

	typedef	std::map<int, std::string>		csvRowTy;
	typedef	std::ofstream				fileTy;
	typedef std::vector<std::string>		csvHeaderTy;

	// Interface and object bring-up/down
	csvWriter(clang::ASTContext *ASTC)
	: IExporter(ASTC), nodeID(0), lastLocFilename(""),
	  lastLocLineNum(~0U) { init(); }
	virtual ~csvWriter() { closeFiles(); }

	void exportDecl(const clang::Decl *D) override;
	void exportStmt(const clang::Stmt *S) override;
	void exportExpr(const clang::Expr *E) override;

	// Utility
	void init();
	void closeFiles();
	void writeNodeRow(csvRowTy &row, fileTy &file);
	void writeNodeRowWrapper();
	void writeHeaders();
	void flushBuffers();
	std::string printSourceRange(clang::SourceRange SR);
	std::string printLocation(clang::SourceLocation SL);

	// Data members
	fileTy	 			nodeFile, edgeFile;
	unsigned long long 		nodeID;
	const char 			*lastLocFilename;
	unsigned 			lastLocLineNum;
	csvRowTy			nodeRowMap;
	csvRowTy			edgeRowMap;
};

} // end of exporter namespace

#endif // CJ_CSV_WRITER_H
