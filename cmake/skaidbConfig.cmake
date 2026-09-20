# find_package(skaidb) for an installed or unpacked libskaidb tree:
#   include/skaidb.h, include/skaidb.hpp, lib/libskaidb.a (and .so/.dylib).
# Point CMAKE_PREFIX_PATH at the tree, then:
#   find_package(skaidb REQUIRED)
#   target_link_libraries(app PRIVATE skaidb::skaidb)
get_filename_component(_skaidb_root "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
if(NOT TARGET skaidb::skaidb)
  add_library(skaidb::skaidb STATIC IMPORTED)
  set_target_properties(skaidb::skaidb PROPERTIES
    IMPORTED_LOCATION "${_skaidb_root}/lib/libskaidb.a"
    INTERFACE_INCLUDE_DIRECTORIES "${_skaidb_root}/include")
  find_package(Threads REQUIRED)
  target_link_libraries(skaidb::skaidb INTERFACE Threads::Threads ${CMAKE_DL_LIBS} m)
endif()
set(skaidb_FOUND TRUE)
