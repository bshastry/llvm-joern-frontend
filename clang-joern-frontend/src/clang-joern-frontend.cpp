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
    explicit ClangASTExporter(ASTContext *Context)
      : Context(Context), cW(Context) {}

    bool VisitDecl(Decl *D) {
      cW.exportDecl(D);
      return true;
    }

    bool VisitStmt(Stmt *S) {
      cW.exportStmt(S);
      return true;
    }

    bool VisitExpr(Expr *E) {
      cW.exportExpr(E);
      return true;
    }

    bool TraverseDecl(Decl *D) {
      // Do something and call base class's traversedecl.
      // The base class's traversedecl actually traverses the AST.
      // Here we only use the entry point for writing an entry into
      // the CSV file(s)
      cW.writeNodeRowWrapper();
      cW.flushBuffers();
      return getBaseRAV().TraverseDecl(D);
    }

    bool TraverseStmt(Stmt *S) {
      cW.writeNodeRowWrapper();
      cW.flushBuffers();
      return getBaseRAV().TraverseStmt(S);
    }

  private:
    ASTContext *Context;
    exporter::csvWriter cW;

    RecursiveASTVisitor<ClangASTExporter> &getBaseRAV() {
      return *static_cast<RecursiveASTVisitor<ClangASTExporter> *>(this);
    }

  };

  class ClangASTExportConsumer : public clang::ASTConsumer {
  public:
    explicit ClangASTExportConsumer(clang::ASTContext *Context)
      : Visitor(Context) {}
    virtual void HandleTranslationUnit(clang::ASTContext &Context) {
      // Traversing the translation unit decl via a RecursiveASTVisitor
      // will visit all nodes in the AST.
      Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }
  private:
    // A RecursiveASTVisitor implementation.
    ClangASTExporter Visitor;
  };

  class ClangASTExportAction : public clang::ASTFrontendAction {
  public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
      clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
      return std::unique_ptr<clang::ASTConsumer>(
          new ClangASTExportConsumer(&Compiler.getASTContext()));
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
