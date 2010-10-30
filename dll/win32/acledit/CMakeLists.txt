
set_unicode()

list(APPEND SOURCE
    acledit.c
    stubs.c
    acledit.rc)

spec2def(acledit.dll acledit.spec)

add_library(acledit SHARED ${SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/acledit.def)

set_module_type(acledit win32dll)

add_importlibs(acledit ntdll)

add_cab_target(acledit 1)
