
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mscat32.dll mscat32.spec)

add_library(mscat32 SHARED main.c ${CMAKE_CURRENT_BINARY_DIR}/mscat32.def)

set_module_type(mscat32 win32dll)

target_link_libraries(mscat32 wine)

add_importlibs(mscat32 wintrust msvcrt kernel32 ntdll)

add_cab_target(mscat32 1)
