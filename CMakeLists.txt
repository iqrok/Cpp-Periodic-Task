cmake_minimum_required(VERSION 3.13)

set(PROJECT_NAME "cycle-test")
set(PROJECT_VERSION "0.0.1")

project("${PROJECT_NAME}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -O2 -fPIC -Wall")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	add_compile_options (-fdiagnostics-color=always)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	add_compile_options (-fcolor-diagnostics)
endif ()

if (DEBUG)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")
endif()

if (DUMMY)
	add_compile_definitions(DUMMY=1)
endif()

message("CXX FLAGS: ${CMAKE_CXX_FLAGS}")

set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build")
set(INCLUDE_SRC_DIR "${SOURCE_DIR}/include")

# link lib pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# link lib rt (realtime), for shm_op and shm_unlink
link_libraries(Threads::Threads rt)

include_directories("${INCLUDE_DIR}" "${INCLUDE_SRC_DIR}")

set(OUTPUT_BIN_MAIN "${PROJECT_NAME}")

set(
		SOURCE_BIN_MAIN
			"${SOURCE_DIR}/main.cpp"
	)

add_executable("${OUTPUT_BIN_MAIN}" "${SOURCE_BIN_MAIN}")
