#include "clang/AST/ASTConsumer.h"
#include "clang/Driver/Options.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/StaticAnalyzer/Frontend/FrontendActions.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Option/OptTable.h"
#include "llvm/Support/Signals.h"

#include "exporters/csv-writer/csv-writer.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;

// Clone of clang-check

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp(
    "\tFor example, to run clang-joern on all files in a subtree of the\n"
    "\tsource tree, use:\n"
    "\n"
    "\t  find path/in/subtree -name '*.cpp'|xargs clang-joern\n"
    "\n"
    "\tor using a specific build path:\n"
    "\n"
    "\t  find path/in/subtree -name '*.cpp'|xargs clang-joern -p build/path\n"
    "\n"
    "\tNote, that path/in/subtree and current directory should follow the\n"
    "\trules described above.\n"
    "\n"
);

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static cl::OptionCategory ClangJoernCategory("clang-joern options");
static std::unique_ptr<opt::OptTable> Options(createDriverOptTable());
static cl::opt<bool>
ASTDump("ast-dump", cl::desc(Options->getOptionHelpText(options::OPT_ast_dump)),
        cl::cat(ClangJoernCategory));
static cl::opt<bool>
ASTList("ast-list", cl::desc(Options->getOptionHelpText(options::OPT_ast_list)),
        cl::cat(ClangJoernCategory));
static cl::opt<bool>
ASTPrint("ast-print",
         cl::desc(Options->getOptionHelpText(options::OPT_ast_print)),
         cl::cat(ClangJoernCategory));
static cl::opt<std::string> ASTDumpFilter(
    "ast-dump-filter",
    cl::desc(Options->getOptionHelpText(options::OPT_ast_dump_filter)),
    cl::cat(ClangJoernCategory));
static cl::opt<bool>
Analyze("analyze", cl::desc(Options->getOptionHelpText(options::OPT_analyze)),
        cl::cat(ClangJoernCategory));
static cl::opt<bool>
ASTExport("ast-export", cl::desc("Export Clang ASTs to Neo4j CSV batch importer format i.e., nodes.csv and edges.csv"),
        cl::cat(ClangJoernCategory));

namespace {
  class ClangASTExporter
    : public RecursiveASTVisitor<ClangASTExporter> {
  public:
    explicit ClangASTExporter(ASTContext *Context, std::string inFile)
      : Context(Context), cW(Context), sourceFilename(inFile) {}

    // Decls
    bool VisitDecl(Decl *D) {
      cW.exportDecl(D);
      return true;
    }

    bool VisitTranslationUnitDecl(TranslationUnitDecl *TUD) {
      cW.exportTranslationUnitDecl(sourceFilename);
      return true;
    }

    bool VisitFunctionDecl(FunctionDecl *FD) {
      return true;
    }

    bool VisitNamedDecl(NamedDecl *ND) {
      cW.exportNamedDecl(ND);
      return true;
    }

    // Stmts
    bool VisitStmt(Stmt *S) {
      cW.exportStmt(S);
      return true;
    }

    bool VisitExpr(Expr *E) {
      cW.exportExpr(E);
      return true;
    }

    bool VisitCastExpr(CastExpr *CE) {
      cW.exportCastExpr(CE);
      return true;
    }

    bool VisitDeclRefExpr(DeclRefExpr *DRE) {
      cW.exportDeclRefExpr(DRE);
      return true;
    }

    // AST nodes in Clang
    // When visiting an AST node using RAV, there is no easy way
    // of knowing when to write out contents of the node into a
    // csv file. So, we defer it to the traverse of the next AST
    // node i.e., we export an AST node before visiting the next
    // AST node. Naturally, the last AST node will get left out.
    // So, we need to explicitly call writeNodeRow() in
    // ClangASTExportConsumer.
    bool TraverseDecl(Decl *D) {
      writeNodeRow();
      return getBaseRAV().TraverseDecl(D);
    }

    bool TraverseStmt(Stmt *S) {
      writeNodeRow();
      return getBaseRAV().TraverseStmt(S);
    }

    bool TraverseType(QualType T) {
      writeNodeRow();
      return getBaseRAV().TraverseType(T);
    }

    bool TraverseTypeLoc(TypeLoc TL) {
      writeNodeRow();
      return getBaseRAV().TraverseTypeLoc(TL);
    }

    bool TraverseAttr(Attr *At) {
      writeNodeRow();
      return getBaseRAV().TraverseAttr(At);
    }

    bool TraverseNestedNameSpecifier(NestedNameSpecifier *NNS) {
      writeNodeRow();
      return getBaseRAV().TraverseNestedNameSpecifier(NNS);
    }

    bool TraverseNestedNameSpecifierLoc(NestedNameSpecifierLoc NNS) {
      writeNodeRow();
      return getBaseRAV().TraverseNestedNameSpecifierLoc(NNS);
    }

    bool TraverseDeclarationNameInfo(DeclarationNameInfo NameInfo) {
      writeNodeRow();
      return getBaseRAV().TraverseDeclarationNameInfo(NameInfo);
    }

    bool TraverseTemplateName(TemplateName Template) {
      writeNodeRow();
      return getBaseRAV().TraverseTemplateName(Template);
    }

    bool TraverseTemplateArgument(const TemplateArgument &Arg) {
      writeNodeRow();
      return getBaseRAV().TraverseTemplateArgument(Arg);
    }

    bool TraverseTemplateArgumentLoc(const TemplateArgumentLoc &ArgLoc) {
      writeNodeRow();
      return getBaseRAV().TraverseTemplateArgumentLoc(ArgLoc);
    }

    bool TraverseTemplateArguments(const TemplateArgument *Args,
                                   unsigned NumArgs) {
      writeNodeRow();
      return getBaseRAV().TraverseTemplateArguments(Args, NumArgs);
    }

    bool TraverseConstructorInitializer(CXXCtorInitializer *Init) {
      writeNodeRow();
      return getBaseRAV().TraverseConstructorInitializer(Init);
    }

    bool TraverseLambdaCapture(LambdaExpr *LE, const LambdaCapture *C) {
      writeNodeRow();
      return getBaseRAV().TraverseLambdaCapture(LE, C);
    }

    bool TraverseLambdaBody(LambdaExpr *LE) {
      writeNodeRow();
      return getBaseRAV().TraverseLambdaBody(LE);
    }

    void writeNodeRow() {
      cW.writeNodeRowWrapper();
    }

  private:
    ASTContext *Context;
    exporter::csvWriter cW;
    std::string sourceFilename;

    RecursiveASTVisitor<ClangASTExporter> &getBaseRAV() {
      return *static_cast<RecursiveASTVisitor<ClangASTExporter> *>(this);
    }

  };

  class ClangASTExportConsumer : public clang::ASTConsumer {
  public:
    explicit ClangASTExportConsumer(clang::ASTContext *Context,
                                    std::string inFile)
      : Visitor(Context, inFile) {}
    virtual void HandleTranslationUnit(clang::ASTContext &Context) {
      // Traversing the translation unit decl via a RecursiveASTVisitor
      // will visit all nodes in the AST.
      Visitor.TraverseDecl(Context.getTranslationUnitDecl());
      // Export the very last AST node in TU
      Visitor.writeNodeRow();
    }
  private:
    // A RecursiveASTVisitor implementation.
    ClangASTExporter Visitor;
  };

  class ClangASTExportAction : public clang::ASTFrontendAction {
  public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
      clang::CompilerInstance &Compiler, llvm::StringRef InFile) override {
      return std::unique_ptr<clang::ASTConsumer>(
          new ClangASTExportConsumer(&Compiler.getASTContext(), InFile));
    }
  };

  // This is a clone of clang-check. Can be removed later.
  class ClangJoernActionFactory {
  public:
    std::unique_ptr<clang::ASTConsumer> newASTConsumer() {
      if (ASTList)
        return clang::CreateASTDeclNodeLister();

      if (ASTDump)
        return clang::CreateASTDumper(ASTDumpFilter, /*DumpDecls=*/true,
                                      /*DumpLookups=*/false);
      if (ASTPrint)
        return clang::CreateASTPrinter(&llvm::outs(), ASTDumpFilter);

      return llvm::make_unique<clang::ASTConsumer>();
    }
  };
} // end of anonymous namespace

int main(int argc, const char **argv) {

  llvm::sys::PrintStackTraceOnErrorSignal();

  CommonOptionsParser OptionsParser(argc, argv, ClangJoernCategory);
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  // Clear adjusters because -fsyntax-only is inserted by the default chain.
  Tool.clearArgumentsAdjusters();
  Tool.appendArgumentsAdjuster(getClangStripOutputAdjuster());

  // Running the analyzer requires --analyze. Other modes can work with the
  // -fsyntax-only option.
  Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster(
      Analyze ? "--analyze" : "-fsyntax-only", ArgumentInsertPosition::BEGIN));

  ClangJoernActionFactory CJFactory;
  std::unique_ptr<FrontendActionFactory> FrontendFactory;

  // Choose the correct factory based on the selected mode.
  if (Analyze)
    FrontendFactory = newFrontendActionFactory<clang::ento::AnalysisAction>();
  else if (ASTExport)
    FrontendFactory = newFrontendActionFactory<ClangASTExportAction>();
  else
    FrontendFactory = newFrontendActionFactory(&CJFactory);

  return Tool.run(FrontendFactory.get());
}
