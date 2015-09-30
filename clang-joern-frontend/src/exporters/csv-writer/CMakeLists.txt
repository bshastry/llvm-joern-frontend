include_directories(${CJ_LLVM_INCLUDE_PATH} ${CJ_CLANG_INCLUDE_PATH}
        "/usr/include/llvm-3.6"
        "exporters"
        )

# Disable RTTI, use C++11 to be compatible with LLVM/Clang libraries.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -std=c++11")

add_library(csvWriter SHARED csv-writer.cpp)
