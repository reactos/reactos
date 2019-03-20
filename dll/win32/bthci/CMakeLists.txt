
spec2def(bthci.dll bthci.spec)

add_library(bthci MODULE
    bthci.c
    bthci.rc
    ${CMAKE_CURRENT_BINARY_DIR}/bthci.def)

set_module_type(bthci win32dll UNICODE)
add_importlibs(bthci msvcrt kernel32 ntdll)
add_cd_file(TARGET bthci DESTINATION reactos/system32 FOR all)
