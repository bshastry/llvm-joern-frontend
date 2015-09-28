#ifndef CJ_CSV_WRITER_H
#define CJ_CSV_WRITER_H

#include <fstream>
#include "IExporter.h"

namespace exporter {

class csvWriter : public IExporter {
public:
	// Typedefs
	// nodeIDTy 	=>	nodeID, typically thousands per TU
	// colIndexTy	=>	csv column index, a non-negative number
	// codePropTy	=>	code property in nodes.csv/edges.csv
	// csvRowTy	=>	<colIndexTy, codePropTy> map in nodes.csv/edges.csv
	// fileTy	=>	std output filestream
	// csvHeaderTy	=>	vector of codePropTy; for nodes/edges.csv headers only
	// declNodeMapTy	=>	decl<->nodeID map
	// stmtNodeMapTy	=>	stmt<->nodeID map
	typedef	unsigned long long			nodeIDTy;
	typedef unsigned				colIndexTy;
	typedef	std::string				codePropTy;
	typedef	std::map<colIndexTy, codePropTy>	csvRowTy;
	typedef	std::ofstream				fileTy;
	typedef std::vector<codePropTy>			csvHeaderTy;
	typedef std::map<const clang::Decl*, nodeIDTy>	declNodeMapTy;
	typedef std::map<const clang::Stmt*, nodeIDTy>	stmtNodeMapTy;

	enum NODECOL : colIndexTy {
	  NODEID	=	0,
	  NODEKIND	=	1,
	  LOC		=	2,
	  LOCRANGE	=	3,
	  TYPE		=	4,
	  VALUEKIND	=	5,
	  VALUE		=	6,
	  CASTKIND	=	7,
	  DECLNAME	=	8,
	  SEMCONTEXT	=	9,
	  LEXCONTEXT	=	10,
	  DECLQUAL	=	11,
	  BAREDECLREF	=	12,
	  FIRST		=	NODEID,
	  LAST		=	BAREDECLREF
	};

	enum EDGECOL : colIndexTy {
	  ENODEID1	=	0,
	  ENODEID2	=	1,
	  ETYPE		=	2,
	  EFIRST	=	ENODEID1,
	  ELAST		=	ETYPE
	};

	enum EDGEREL : colIndexTy {
	  IS_PARENT_OF		=	0,
	  SEMANTIC_PARENT	=	1,
	  DECLREF_EXPR		=	2,
	  NUM_RELS		=	DECLREF_EXPR + 1
	};

	const char *EDGERELKEYS[NUM_RELS] = {
	    "is_parent_of",
	    "semantic_parent",
	    "references_decl"
	};

	// Constructor and copy set
	csvWriter(clang::ASTContext *ASTC)
	: IExporter(ASTC), nodeID(0), lastLocFilename(""),
	  lastLocLineNum(~0U) { init(); }
	virtual ~csvWriter() { closeFiles(); }
	csvWriter(const csvWriter &copy) = delete;
	csvWriter &operator=(const csvWriter &rhs) = delete;

	void exportDecl(const clang::Decl *D) override;
	void exportNamedDecl(const clang::NamedDecl *ND) override;
	void exportTranslationUnitDecl(std::string filename) override;
	void exportStmt(const clang::Stmt *S) override;
	void exportExpr(const clang::Expr *E) override;
	void exportCastExpr(const clang::CastExpr *E) override;
	void exportDeclRefExpr(const clang::DeclRefExpr *DRE) override;

	// Utility
	void init();
	void closeFiles();
	void writeRow(csvRowTy &row, fileTy &file, colIndexTy start,
	                  colIndexTy end);
	void writeEdgeRow(codePropTy node1, codePropTy node2, codePropTy rel);
	void writeNodeRowWrapper();
	void writeEdgeRowWrapper();
	void writeHeaders();
	void flushBuffers();
	codePropTy getNodeIDFromDeclPtr(const clang::Decl *D);
	codePropTy getNodeIDFromStmtPtr(const clang::Stmt *S);
	void writeParentChildEdges(const
	    llvm::ArrayRef<clang::ast_type_traits::DynTypedNode> &parentsOfNode);

	// ASTDumper
	codePropTy getSourceRange(clang::SourceRange SR);
	codePropTy getLocation(clang::SourceLocation SL);
	codePropTy getBareType(clang::QualType T, bool Desugar = true);
	codePropTy getType(clang::QualType T);
//	codePropTy exportTypeAsChild(clang::QualType T);
//	codePropTy exportTypeAsChild(const clang::Type *T);
	codePropTy getBareDeclRef(const clang::Decl *D);
	codePropTy getDeclQual(const clang::Decl *D);
//	codePropTy exportDeclRef(const clang::Decl *Node, const char *Label = nullptr);
//	bool hasNodes(const clang::DeclContext *DC);
//	codePropTy exportDeclContext(const clang::DeclContext *DC);
//	codePropTy exportLookups(const clang::DeclContext *DC, bool exportDecls);
//	codePropTy exportAttr(const clang::Attr *A);
	// ASTDumper

	// Data members
	fileTy	 			nodeFile, edgeFile;
	nodeIDTy	 		nodeID;
	const char 			*lastLocFilename;
	unsigned 			lastLocLineNum;
	csvRowTy			nodeRowMap;
	csvRowTy			edgeRowMap;
	declNodeMapTy			declNodeMap;
	stmtNodeMapTy			stmtNodeMap;
};

} // end of exporter namespace

#endif // CJ_CSV_WRITER_H
