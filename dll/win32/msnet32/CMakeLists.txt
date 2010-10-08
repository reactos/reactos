
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(msnet32 ${CMAKE_CURRENT_SOURCE_DIR}/msnet32.spec ${CMAKE_CURRENT_BINARY_DIR}/msnet32.def)

add_library(msnet32 SHARED msnet_main.c)

set_module_type(msnet32 win32dll)

target_link_libraries(msnet32
    ${CMAKE_CURRENT_BINARY_DIR}/msnet32.def
    wine)

add_importlibs(msnet32 kernel32 ntdll)
add_dependencies(msnet32 msnet32_def)
