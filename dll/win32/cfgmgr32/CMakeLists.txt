
list(APPEND SOURCE
    cfgmgr32.rc)

add_library(cfgmgr32 MODULE ${SOURCE})
spec2def(cfgmgr32 cfgmgr32.dll cfgmgr32.spec)

set_module_type(cfgmgr32 win32dll)
add_importlibs(cfgmgr32 setupapi msvcrt kernel32 ntdll)
add_cd_file(TARGET cfgmgr32 DESTINATION reactos/system32 FOR all)
