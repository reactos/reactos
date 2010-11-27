
spec2def(msimg32.dll msimg32.spec)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

add_library(msimg32 SHARED
            msimg32_main.c
            ${CMAKE_CURRENT_BINARY_DIR}/msimg32.def)

set_module_type(msimg32 win32dll)

target_link_libraries(msimg32 wine)

add_importlibs(msimg32 gdi32 kernel32 ntdll)

add_cab_target(msimg32 1)
add_importlib_target(msimg32.spec)
