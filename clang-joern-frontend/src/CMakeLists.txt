cmake_minimum_required(VERSION 2.8)

project(clang-joern)

if( CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR )
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

  if( MSVC )
    # Adjust MSVC warning levels.
    add_definitions(
      # Ignore security warnings for standard functions.
      -D_CRT_SECURE_NO_WARNINGS
      -D_SCL_SECURE_NO_WARNINGS

      # Disabled warnings.
      -wd4146 # Suppress 'unary minus operator applied to unsigned type, result still unsigned'
      -wd4244 # Suppress ''argument' : conversion from 'type1' to 'type2', possible loss of data'
      -wd4291 # Suppress ''declaration' : no matching operator delete found; memory will not be freed if initialization throws an exception'
      -wd4345 # Suppress 'behavior change: an object of POD type constructed with an initializer of the form () will be default-initialized'
      -wd4355 # Suppress ''this' : used in base member initializer list'
      -wd4624 # Suppress ''derived class' : destructor could not be generated because a base class destructor is inaccessible'
      -wd4800 # Suppress ''type' : forcing value to bool 'true' or 'false' (performance warning)'

      # Promoted warnings.
      -w14062 # Promote 'enumerator in switch of enum is not handled' to level 1 warning.

      # Promoted warnings to errors.
      -we4238 # Promote 'nonstandard extension used : class rvalue used as lvalue' to error.
    )
  endif()

  if( CMAKE_SYSTEM_NAME MATCHES "Darwin" )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")
  endif()
else()
  message(STATUS "Clang-Joern in-tree configuration")
endif()

add_executable(clang-joern
clang-joern-frontend.cpp
)
if( MSVC )
  # Disable warnings for CJ, and disable exceptions in MSVC's STL.
  add_definitions(
    -wd4722 # Suppress ''destructor'' : destructor never returns, potential memory leak
    -D_HAS_EXCEPTIONS=0
  )

  # Put project in solution folder
  set_target_properties(clang-joern
    PROPERTIES FOLDER "Clang executables"
  )
else()
  # Disable RTTI, use C++11 to be compatible with LLVM/Clang libraries.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -std=c++11")
endif()

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

# Platform dependencies.
if( CMAKE_SYSTEM_NAME MATCHES "Windows" )
  target_link_libraries(clang-joern
  )
else()
  # POSIX
  include(FindCurses)
  target_link_libraries(clang-joern
    pthread
    z
    dl
    ${CURSES_LIBRARIES}
  )

  # Only Linux targets need dl for dynamic linking.
  if( CMAKE_SYSTEM_NAME MATCHES "Linux" )
    target_link_libraries(clang-joern
      dl
    )
  endif()
endif()

install(TARGETS clang-joern
  RUNTIME DESTINATION bin)
