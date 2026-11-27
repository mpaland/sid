# sid - Sound Interface Driver library
#
# Header-only: nothing is built for the library itself. The target sid::sid gives the include path and, on
# Windows, the system library of the sound output (port/win32.h).
#
#   add_subdirectory(sid)                      # or FetchContent, see readme.md
#   target_link_libraries(app PRIVATE sid::sid)
#   #include "sid/src/synth.h"
#
# As the top project it builds the tests and the examples:
#   cmake -S . -B build
#   cmake --build build --config Release
#   ctest --test-dir build -C Release --output-on-failure

cmake_minimum_required(VERSION 3.18)

project(sid
  VERSION      1.1.0    # x-release-please-version
  DESCRIPTION  "Sound Interface Driver library - the sound of the 80s home computers for small systems"
  HOMEPAGE_URL "https://github.com/mpaland/sid"
  LANGUAGES    CXX
)

add_library(sid INTERFACE)
add_library(sid::sid ALIAS sid)
target_compile_features(sid INTERFACE cxx_std_11)

# The headers are included as "sid/src/<name>.h" - the way they are found when this repository is a folder named
# sid in a project (the parent folder on the include path). So that this form works whatever the folder is called
# and wherever it is, a header of that path that forwards to the real one is made for every header
set(SID_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
file(GLOB SID_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" CONFIGURE_DEPENDS src/*.h src/port/*.h src/ext/*.h)
foreach(header ${SID_HEADERS})
  file(CONFIGURE
    OUTPUT  "${SID_INCLUDE_DIR}/sid/${header}"
    CONTENT "#include \"${CMAKE_CURRENT_SOURCE_DIR}/${header}\"\n"
    @ONLY
  )
endforeach()
target_include_directories(sid INTERFACE "${SID_INCLUDE_DIR}")
if(WIN32)
  target_link_libraries(sid INTERFACE winmm)
endif()

# tests and examples: on when sid is the top project
string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" SID_IS_TOP)
option(SID_BUILD_TESTS    "Build the tests of sid"                ${SID_IS_TOP})
option(SID_BUILD_EXAMPLES "Build the examples of sid"             ${SID_IS_TOP})
option(SID_WERROR         "Warnings are errors (tests, examples)" OFF)

# C++11 is all the library needs; another standard: -DCMAKE_CXX_STANDARD=20
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 11)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(sid_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    if(SID_WERROR)
      target_compile_options(${target} PRIVATE /WX)
    endif()
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wshadow -Wpedantic)
    if(SID_WERROR)
      target_compile_options(${target} PRIVATE -Werror)
    endif()
  endif()
endfunction()

if(SID_BUILD_TESTS OR SID_BUILD_EXAMPLES)
  enable_testing()
endif()
if(SID_BUILD_TESTS)
  add_subdirectory(test)
endif()
if(SID_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
