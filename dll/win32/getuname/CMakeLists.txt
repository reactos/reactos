
set_unicode()

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(getuname SHARED
    getuname.c
    getuname.rc
    ${CMAKE_CURRENT_BINARY_DIR}/getuname.def)

set_export_spec(getuname getuname.spec)

add_importlib_target(${CMAKE_CURRENT_BINARY_DIR}/getuname.def)

set_module_type(getuname win32dll)

add_importlibs(getuname kernel32 ntdll)

add_cab_target(getuname 1)
