


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(query ${CMAKE_CURRENT_SOURCE_DIR}/query.spec ${CMAKE_CURRENT_BINARY_DIR}/query.def)

add_library(query SHARED query_main.c)

set_target_properties(query PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(query
    ${CMAKE_CURRENT_BINARY_DIR}/query.def
    wine
    -luuid
    mingw_common)

add_dependencies(query query_def psdk)
