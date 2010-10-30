
add_definitions(-D__WINESRC__)
add_definitions(-D_DLL -D__USE_CRTIMP)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(odbc32 ${CMAKE_CURRENT_SOURCE_DIR}/odbc32.spec ${CMAKE_CURRENT_BINARY_DIR}/odbc32.def)

add_library(odbc32 SHARED proxyodbc.c)

set_module_type(odbc32 win32dll)

target_link_libraries(odbc32
    ${CMAKE_CURRENT_BINARY_DIR}/odbc32.def
    wine)

add_importlibs(odbc32
    msvcrt
    advapi32
    kernel32
    ntdll)

add_dependencies(odbc32 odbc32_def)
add_cab_target(odbc32 1)
