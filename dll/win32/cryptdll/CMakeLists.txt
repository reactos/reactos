
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(cryptdll ${CMAKE_CURRENT_SOURCE_DIR}/cryptdll.spec ${CMAKE_CURRENT_BINARY_DIR}/cryptdll.def)

add_library(cryptdll SHARED cryptdll.c)

set_module_type(cryptdll win32dll)

target_link_libraries(cryptdll
    ${CMAKE_CURRENT_BINARY_DIR}/cryptdll.def
    wine)

add_importlibs(cryptdll kernel32 ntdll)
add_dependencies(cryptdll cryptdll_def)
add_livecd_target(cryptdll reactos/system32)
