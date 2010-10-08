
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(msimg32 ${CMAKE_CURRENT_SOURCE_DIR}/msimg32.spec ${CMAKE_CURRENT_BINARY_DIR}/msimg32.def)

add_library(msimg32 SHARED msimg32_main.c)

set_module_type(msimg32 win32dll)

target_link_libraries(msimg32
    ${CMAKE_CURRENT_BINARY_DIR}/msimg32.def
    wine)

add_importlibs(msimg32 gdi32 kernel32 ntdll)
add_dependencies(msimg32 msimg32_def)
