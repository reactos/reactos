
spec2def(cfgmgr32.dll cfgmgr32.spec)

list(APPEND SOURCE
    cfgmgr32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/cfgmgr32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/cfgmgr32.def)

add_library(cfgmgr32 SHARED ${SOURCE})

set_module_type(cfgmgr32 win32dll)
add_importlibs(cfgmgr32 msvcrt setupapi kernel32 ntdll)
add_dependencies(cfgmgr32 psdk)
add_cd_file(TARGET cfgmgr32 DESTINATION reactos/system32 FOR all)
