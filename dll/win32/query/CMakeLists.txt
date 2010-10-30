
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(query.dll query.spec)

add_library(query SHARED query_main.c ${CMAKE_CURRENT_BINARY_DIR}/query.def)

set_module_type(query win32dll)

target_link_libraries(query
    wine
    uuid)

add_importlibs(query kernel32 ntdll)

add_cab_target(query 1)
