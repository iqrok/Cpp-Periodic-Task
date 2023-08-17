cmake_minimum_required(VERSION 3.13)

set(PROJECT_NAME "cycle-test")
set(PROJECT_VERSION "0.0.1")

project("${PROJECT_NAME}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -O2")

add_compile_options(-fPIC -Wall -Wfatal-errors -Wpedantic)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	add_compile_options (-fdiagnostics-color=always)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	add_compile_options (-fcolor-diagnostics)
endif ()

if ("${ARCH}" STREQUAL "arm64" OR "${ARCH}" STREQUAL "aarch64")
	set(ARCH "aarch64")
	set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
	set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
elseif ("${ARCH}" STREQUAL "armhf")
	set(ARCH "armhf")
	set(CMAKE_C_COMPILER   arm-linux-gnueabihf-gcc)
	set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
else ()
	set(ARCH "x86_64")
	set(CMAKE_C_COMPILER   "cc")
	set(CMAKE_CXX_COMPILER "c++")
endif ()

#~ if (!ARCH)
	#~ set(CMAKE_C_COMPILER   "cc")
	#~ set(CMAKE_CXX_COMPILER "c++")
#~ endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${ARCH}")

if (DEBUG)
	add_compile_options(-ggdb)
endif()

set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build")

# link lib pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

link_libraries(Threads::Threads)

include_directories("${INCLUDE_DIR}")

message("[CC COMPILER] ${CMAKE_C_COMPILER}")
message("[CXX COMPILER] ${CMAKE_CXX_COMPILER}")
message("[CXX FLAGS] ${CMAKE_CXX_FLAGS}")

add_subdirectory("./test")
