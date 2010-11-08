
set_unicode()

list(APPEND SOURCE
    acledit.c
    stubs.c
    acledit.rc)

add_library(acledit SHARED ${SOURCE})

set_module_type(acledit win32dll)

set_pdef_file(acledit acledit.pdef)

add_importlibs(acledit ntdll)

add_cab_target(acledit 1)

add_importlib_target(${CMAKE_CURRENT_BINARY_DIR}/acledit.def)
