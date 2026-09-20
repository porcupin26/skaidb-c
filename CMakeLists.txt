# skaidb C/C++ driver — CMake entry point.
#
#   cmake -S . -B build && cmake --build build && ctest --test-dir build
#
# By default the library is built from the pinned skaidb-rust tag via
# scripts/build.sh (needs cargo). Set -DSKAIDB_PREBUILT=<dir> to use a
# downloaded release archive instead (its include/ and lib/), which needs no
# Rust toolchain at all.
cmake_minimum_required(VERSION 3.16)
file(STRINGS SKAIDB_VERSION SKAIDB_PINNED_VERSION LIMIT_COUNT 1)
project(skaidb-c VERSION ${SKAIDB_PINNED_VERSION} LANGUAGES C CXX)

set(SKAIDB_PREBUILT "" CACHE PATH "Directory holding include/ and lib/ from a libskaidb release archive")
option(SKAIDB_BUILD_EXAMPLES "Build the examples" ON)
option(SKAIDB_BUILD_TESTS "Build the smoke tests (need a server; see tests/README.md)" ON)

if(SKAIDB_PREBUILT)
  set(SKAIDB_DIST "${SKAIDB_PREBUILT}")
else()
  set(SKAIDB_DIST "${CMAKE_BINARY_DIR}/dist")
  add_custom_command(
    OUTPUT "${SKAIDB_DIST}/lib/libskaidb.a"
    COMMAND "${CMAKE_SOURCE_DIR}/scripts/build.sh" "${SKAIDB_DIST}"
    DEPENDS "${CMAKE_SOURCE_DIR}/SKAIDB_VERSION"
    COMMENT "Building libskaidb ${SKAIDB_PINNED_VERSION} with cargo")
  add_custom_target(skaidb_build DEPENDS "${SKAIDB_DIST}/lib/libskaidb.a")
endif()

add_library(skaidb::skaidb STATIC IMPORTED GLOBAL)
set_target_properties(skaidb::skaidb PROPERTIES
  IMPORTED_LOCATION "${SKAIDB_DIST}/lib/libskaidb.a"
  INTERFACE_INCLUDE_DIRECTORIES "${SKAIDB_DIST}/include")
find_package(Threads REQUIRED)
target_link_libraries(skaidb::skaidb INTERFACE Threads::Threads ${CMAKE_DL_LIBS} m)
if(NOT SKAIDB_PREBUILT)
  add_dependencies(skaidb::skaidb skaidb_build)
endif()

if(SKAIDB_BUILD_EXAMPLES)
  add_executable(basic_usage_c examples/basic_usage.c)
  target_link_libraries(basic_usage_c PRIVATE skaidb::skaidb)
  set_property(TARGET basic_usage_c PROPERTY C_STANDARD 11)
  add_executable(basic_usage_cpp examples/basic_usage.cpp)
  target_link_libraries(basic_usage_cpp PRIVATE skaidb::skaidb)
  set_property(TARGET basic_usage_cpp PROPERTY CXX_STANDARD 17)
endif()

if(SKAIDB_BUILD_TESTS)
  enable_testing()
  add_executable(smoke_c tests/smoke.c)
  target_link_libraries(smoke_c PRIVATE skaidb::skaidb)
  set_property(TARGET smoke_c PROPERTY C_STANDARD 11)
  add_executable(smoke_cpp tests/smoke.cpp)
  target_link_libraries(smoke_cpp PRIVATE skaidb::skaidb)
  set_property(TARGET smoke_cpp PROPERTY CXX_STANDARD 17)
  add_test(NAME smoke_c COMMAND smoke_c)
  add_test(NAME smoke_cpp COMMAND smoke_cpp)
endif()

# `cmake --install` lays out the same tree a release archive holds.
install(DIRECTORY "${SKAIDB_DIST}/include/" DESTINATION include)
install(FILES "${SKAIDB_DIST}/lib/libskaidb.a" DESTINATION lib)
install(FILES cmake/skaidbConfig.cmake DESTINATION lib/cmake/skaidb)
