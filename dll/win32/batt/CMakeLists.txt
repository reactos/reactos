
spec2def(batt.dll batt.spec)

add_library(batt MODULE
    batt.c
    batt.rc
    ${CMAKE_CURRENT_BINARY_DIR}/batt.def)

set_module_type(batt win32dll UNICODE)
add_importlibs(batt setupapi msvcrt kernel32 ntdll)
add_cd_file(TARGET batt DESTINATION reactos/system32 FOR all)
