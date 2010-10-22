
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(query ${CMAKE_CURRENT_SOURCE_DIR}/query.spec ${CMAKE_CURRENT_BINARY_DIR}/query.def)

add_library(query SHARED query_main.c)

set_module_type(query win32dll)

target_link_libraries(query
    ${CMAKE_CURRENT_BINARY_DIR}/query.def
    wine
    uuid)

add_importlibs(query kernel32 ntdll)
add_dependencies(query query_def)
add_livecd_target(query reactos/system32)
