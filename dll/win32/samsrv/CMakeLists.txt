
set_unicode()

spec2def(samsrv.dll samsrv.spec)

add_library(samsrv SHARED samsrv.c samsrv.rc
    ${CMAKE_CURRENT_BINARY_DIR}/samsrv.def)

set_entrypoint(samsrv 0)

add_importlibs(samsrv kernel32 ntdll)
add_dependencies(samsrv psdk buildno_header)
