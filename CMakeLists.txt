cmake_minimum_required(VERSION 3.8...3.10)

project(espeak-ng
  VERSION 1.52.0.1
  DESCRIPTION "open source speech synthesizer that supports more than hundred languages and accents"
  HOMEPAGE_URL "https://github.com/espeak-ng/espeak-ng"
)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(COMPILE_INTONATIONS "Intonation compilation" OFF)
option(ENABLE_TESTS "Enable tests" ON)
option(WITH_COVERAGE "Enable clang source-based coverage instrumentation" OFF)
option(WITH_FUZZER "Build libFuzzer targets (requires Clang)" OFF)

include(CTest)

if (WITH_COVERAGE OR WITH_FUZZER)
  if (NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR "WITH_COVERAGE and WITH_FUZZER require Clang")
  endif()
endif()

if (WITH_COVERAGE)
  add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
endif()

include(cmake/deps.cmake)
include(cmake/config.cmake)
add_subdirectory(src)

if (COMPILE_INTONATIONS)
  if (CMAKE_CROSSCOMPILING)
    find_program(NATIVEBUILD NAMES espeak-ng PATHS ${NativeBuild_DIR} NO_DEFAULT_PATH)
    if(NATIVEBUILD)
      message(STATUS "Native build of espeak-ng is located at: ${NATIVEBUILD}")
      include(cmake/data.cmake)
    else()
      message(STATUS "Not building intonations as the build is a cross compile.")
      message(STATUS "Call CMake with the location of the native build of espeak-ng")
      message(STATUS "  e.g. -DNativeBuild=~/src/build-native")
      message(STATUS "to build the intonations.")
    endif()
  else()
    include(cmake/data.cmake)
  endif()
endif()

# Copy pre-compiled espeak-ng-data to build directory
file(COPY ${CMAKE_SOURCE_DIR}/espeak-ng-data DESTINATION ${CMAKE_BINARY_DIR})
message(STATUS "Copied espeak-ng-data to build directory")

include(cmake/docs.cmake)

include(cmake/package.cmake)
include(CPack)



message(STATUS "Configuration:")
message(STATUS "  shared: ${BUILD_SHARED_LIBS}")
