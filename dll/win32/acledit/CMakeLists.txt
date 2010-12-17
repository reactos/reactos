
set_unicode()

spec2def(acledit.dll acledit.spec)

list(APPEND SOURCE
    acledit.c
    stubs.c
    acledit.rc
    ${CMAKE_CURRENT_BINARY_DIR}/acledit.def)

add_library(acledit SHARED ${SOURCE})

set_module_type(acledit win32dll)
add_importlibs(acledit msvcrt kernel32 ntdll)
add_cab_target(acledit 1)
add_importlib_target(acledit.spec)
