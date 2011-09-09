

spec2def(samsrv.dll samsrv.spec)

list(APPEND SOURCE
    samsrv.c
    samsrv.rc
    ${CMAKE_CURRENT_BINARY_DIR}/samsrv.def)

add_library(samsrv SHARED ${SOURCE})

set_module_type(samsrv win32dll ENTRYPOINT 0 UNICODE)

add_importlibs(samsrv kernel32 ntdll)
add_dependencies(samsrv psdk)
add_cd_file(TARGET samsrv DESTINATION reactos/system32 FOR all)
