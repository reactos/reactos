
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

add_library(dplay MODULE
    version.rc)
spec2def(dplay dplay.dll dplay.spec)

set_module_type(dplay win32dll)
add_importlibs(dplay dplayx msvcrt kernel32)
add_cd_file(TARGET dplay DESTINATION reactos/system32 FOR all)
