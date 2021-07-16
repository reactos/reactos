
spec2def(clb.dll clb.spec)

add_library(clb MODULE
    clb.c
    clb.rc
    ${CMAKE_CURRENT_BINARY_DIR}/clb.def)

set_module_type(clb win32dll UNICODE)
add_importlibs(clb user32 comctl32 msvcrt kernel32 ntdll)
add_cd_file(TARGET clb DESTINATION reactos/system32 FOR all)
