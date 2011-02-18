
spec2def(batt.dll batt.spec)

set_unicode()

add_library(batt SHARED
    batt.c
    batt.rc
    ${CMAKE_CURRENT_BINARY_DIR}/batt.def)

set_module_type(batt win32dll)

add_importlib_target(batt.spec)

add_importlibs(batt msvcrt kernel32 ntdll)

add_cab_target(batt 1)
