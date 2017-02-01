
spec2def(clb.dll clb.spec)

add_library(clb SHARED
    clb.c
    clb.rc
    ${CMAKE_CURRENT_BINARY_DIR}/clb.def)

set_module_type(clb win32dll UNICODE)
add_importlibs(clb user32 comctl32 msvcrt kernel32 ntdll)
