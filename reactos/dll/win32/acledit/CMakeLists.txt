
set_rc_compiler()
spec2def(acledit.dll acledit.spec)

list(APPEND SOURCE
    acledit.c
    stubs.c
    acledit.rc
    ${CMAKE_CURRENT_BINARY_DIR}/acledit.def)

add_library(acledit SHARED ${SOURCE})

set_module_type(acledit win32dll UNICODE)
add_importlibs(acledit msvcrt kernel32 ntdll)
add_pch(acledit acleditint.h)
add_cd_file(TARGET acledit DESTINATION reactos/system32 FOR all)

