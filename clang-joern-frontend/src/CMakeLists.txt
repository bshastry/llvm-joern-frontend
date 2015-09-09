cmake_minimum_required(VERSION 2.8)

project(clang-joern)

message(STATUS "Clang-Joern out-of-tree configuration")

if( DEFINED LLVM_PATH )
    set(CJ_LLVM_ROOT_PATH ${LLVM_PATH})
    message(WARNING "LLVM_PATH is deprecated, use CJ_LLVM_ROOT_PATH and CJ_LLVM_BUILD_ROOT_PATH instead.")
endif()

if( DEFINED CJ_LLVM_INCLUDE_PATH AND DEFINED CJ_LLVM_LIB_PATH )
  # User provided include/lib paths, fall through
elseif ( DEFINED CJ_LLVM_ROOT_PATH AND DEFINED CJ_LLVM_BUILD_ROOT_PATH )
    # Synthesize include/lib relative to a root.
  set(CJ_LLVM_INCLUDE_PATH ${CJ_LLVM_ROOT_PATH}/include)
  set(CJ_LLVM_LIB_PATH ${CJ_LLVM_BUILD_ROOT_PATH}/lib)
  set(CJ_CLANG_INCLUDE_PATH ${CJ_LLVM_ROOT_PATH}/tools/clang/include
  			    ${CJ_LLVM_BUILD_ROOT_PATH}/tools/clang/include)
else()
  # If none provided, fail.
  message(FATAL_ERROR
      "Don't know how to find LLVM headers/libraries. "
      "Use -DCJ_LLVM_ROOT_PATH=/xyz and -DCJ_LLVM_BUILD_ROOT_PATH=/xyz_build or both "
      "-DCJ_LLVM_INCLUDE_PATH=/xyz/include and -DCJ_LLVM_LIB_PATH=/xyz/lib")
endif()

include_directories(${CJ_LLVM_INCLUDE_PATH} ${CJ_CLANG_INCLUDE_PATH}
	"/usr/include/llvm-3.6")
link_directories(${CJ_LLVM_LIB_PATH})

add_definitions(
  -D__STDC_LIMIT_MACROS
  -D__STDC_CONSTANT_MACROS
)

add_executable(clang-joern
clang-joern-frontend.cpp
)

# Disable RTTI, use C++11 to be compatible with LLVM/Clang libraries.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -std=c++11")

# Clang dependencies.
target_link_libraries(clang-joern
  clangFrontend
  clangSerialization
  clangDriver
  clangParse
  clangSema
  clangAnalysis
  clangAST
  clangBasic
  clangEdit
  clangLex
  clangTooling
)

# LLVM dependencies.
target_link_libraries(clang-joern
#  LLVMX86AsmParser # MC, MCParser, Support, X86Desc, X86Info
#  LLVMX86Desc # MC, Support, X86AsmPrinter, X86Info
#  LLVMX86AsmPrinter # MC, Support, X86Utils
#  LLVMX86Info # MC, Support, Target
#  LLVMX86Utils # Core, Support
#  LLVMipo
#  LLVMScalarOpts
#  LLVMInstCombine
#  LLVMTransformUtils
#  LLVMipa
#  LLVMAnalysis
#  LLVMTarget
  LLVMOption # Support
  LLVMMCParser # MC, Support
  LLVMMC # Object, Support
#  LLVMObject # BitReader, Core, Support
  LLVMBitReader # Core, Support
#  LLVMCore # Support
  LLVMSupport
)

# POSIX
set(CMAKE_INSTALL_PREFIX "/usr")
target_link_libraries(clang-joern
    pthread
    z
    dl
    ncurses
)

install(TARGETS clang-joern
  RUNTIME DESTINATION bin)
