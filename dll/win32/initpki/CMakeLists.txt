
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(initpki ${CMAKE_CURRENT_SOURCE_DIR}/initpki.spec ${CMAKE_CURRENT_BINARY_DIR}/initpki.def)

add_library(initpki SHARED main.c)

set_module_type(initpki win32dll)

target_link_libraries(initpki
    ${CMAKE_CURRENT_BINARY_DIR}/initpki.def
    wine)

add_importlibs(initpki kernel32 ntdll)
add_dependencies(initpki initpki_def)
add_livecd_target(initpki reactos/system32)
