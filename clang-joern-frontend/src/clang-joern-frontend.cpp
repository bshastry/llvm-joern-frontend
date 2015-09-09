#include "clang/AST/ASTConsumer.h"
#include "clang/Driver/Options.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/StaticAnalyzer/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Option/OptTable.h"
#include "llvm/Support/Signals.h"

using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;

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

namespace {
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
}

int main(int argc, const char **argv) {

  llvm::sys::PrintStackTraceOnErrorSignal();

  // Initialize targets for clang module support.
//  llvm::InitializeAllTargets();
//  llvm::InitializeAllTargetMCs();
//  llvm::InitializeAllAsmPrinters();
//  llvm::InitializeAllAsmParsers();

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
//  if (Analyze)
//    FrontendFactory = newFrontendActionFactory<clang::ento::AnalysisAction>();
//  else
    FrontendFactory = newFrontendActionFactory(&CJFactory);

  return Tool.run(FrontendFactory.get());

//  return Tool.run(newFrontendActionFactory<clang::SyntaxOnlyAction>().get());
}
