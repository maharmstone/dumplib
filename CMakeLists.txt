cmake_minimum_required(VERSION 3.10)

project(dumplib)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(SRC_FILES
	src/dumplib.cpp)

add_executable(dumplib ${SRC_FILES})

if(NOT MSVC)
	target_compile_options(dumplib PUBLIC -Wall -Werror=cast-function-type -Wno-expansion-to-defined -Wunused-parameter -Wtype-limits -Wextra -Wconversion)
endif()
