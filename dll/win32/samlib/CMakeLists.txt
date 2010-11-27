
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(samlib.dll samlib.spec)

add_library(samlib SHARED dllmain.c samlib.c samlib.rc ${CMAKE_CURRENT_BINARY_DIR}/samlib.def)

set_module_type(samlib win32dll)

add_importlibs(samlib advapi32 msvcrt kernel32 ntdll)

add_cab_target(samlib 1)
add_importlib_target(samlib.spec)
