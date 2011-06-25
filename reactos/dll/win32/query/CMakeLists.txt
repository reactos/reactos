
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(query.dll query.spec GENERATE_STUBS)

list(APPEND SOURCE
    query_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/query_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/query.def)

add_library(query SHARED ${SOURCE})

set_module_type(query win32dll)

target_link_libraries(query
    wine
    uuid)

add_importlibs(query msvcrt kernel32 ntdll)

add_cd_file(TARGET query DESTINATION reactos/system32 FOR all)
