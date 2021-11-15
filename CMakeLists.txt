cmake_minimum_required(VERSION 3.4...3.18)
project(mgpybind)

set(MEM-GREP-ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(MEM-GREP-OUT "${MEM-GREP-ROOT}/out")

#pthreads on Unix-like systems
FIND_PACKAGE (Threads REQUIRED)

option(BUILD_UNIT_TESTS "Build the unit tests and associated programs" OFF)
if (BUILD_UNIT_TESTS)
	add_subdirectory(tests)
endif()


option(BUILD_WITH_SANITISER "Build with address and UB sanitisation" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "-g -Werror -Wall -Wextra -Wreorder -Weffc++ -Wpedantic -Wimplicit-fallthrough")
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    #Force clang to build std lib function in debug. Otherwise GDB can't see stdlib objects properly
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG")
endif ()
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -march=native")
set(EXECUTABLE_OUTPUT_PATH "${MEM-GREP-OUT}")
if (BUILD_WITH_SANITISER)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address,undefined -fno-omit-frame-pointer")
endif()

add_subdirectory(pybind11)
pybind11_add_module(mgpybind bindings/bindings.cpp
        mem-grep/lib/misc/map-parser.cpp mem-grep/lib/misc/map-parser.hpp
		mem-grep/lib/heap-traversing/bss-searcher.cpp mem-grep/lib/heap-traversing/bss-searcher.hpp
		mem-grep/lib/misc/malloc-metadata.cpp mem-grep/lib/misc/malloc-metadata.hpp
		mem-grep/lib/misc/prerun-checks.cpp mem-grep/lib/misc/prerun-checks.hpp
		mem-grep/lib/heap-traversing/stack-searcher.cpp mem-grep/lib/heap-traversing/stack-searcher.hpp
		mem-grep/lib/heap-traversing/heap-traverser.cpp mem-grep/lib/heap-traversing/heap-traverser.hpp
		mem-grep/lib/misc/structs.cpp mem-grep/lib/misc/structs.hpp
		mem-grep/lib/misc/remote-memory.cpp mem-grep/lib/misc/remote-memory.hpp
		mem-grep/lib/filtering/heap-filter.cpp mem-grep/lib/filtering/heap-filter.hpp
		mem-grep/lib/filtering/lambda-creator.cpp mem-grep/lib/filtering/lambda-creator.hpp
		mem-grep/lib/misc/utils.hpp mem-grep/lib/misc/utils.cpp
		mem-grep/lib/heap-traversing/bool-vec.hpp mem-grep/lib/heap-traversing/bool-vec.cpp
)




target_compile_definitions(mgpybind PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO})
