
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(mssign32 ${CMAKE_CURRENT_SOURCE_DIR}/mssign32.spec ${CMAKE_CURRENT_BINARY_DIR}/mssign32.def)

add_library(mssign32 SHARED mssign32_main.c)

set_module_type(mssign32 win32dll)

target_link_libraries(mssign32
    ${CMAKE_CURRENT_BINARY_DIR}/mssign32.def
    wine)

add_importlibs(mssign32 kernel32 ntdll)
add_dependencies(mssign32 mssign32_def)
add_cab_target(mssign32 1)
