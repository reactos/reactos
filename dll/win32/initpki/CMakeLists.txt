
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(initpki.dll initpki.spec)

add_library(initpki SHARED main.c ${CMAKE_CURRENT_BINARY_DIR}/initpki.def)

set_module_type(initpki win32dll)

target_link_libraries(initpki wine)

add_importlibs(initpki msvcrt kernel32 ntdll)

add_cab_target(initpki 1)
