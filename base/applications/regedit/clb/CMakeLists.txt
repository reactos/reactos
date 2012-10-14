
spec2def(clb.dll clb.spec)

set_rc_compiler()

add_library(clb SHARED
    clb.c
    clb.rc
    ${CMAKE_CURRENT_BINARY_DIR}/clb.def)

set_module_type(clb win32dll UNICODE)
add_importlibs(clb user32 gdi32 comctl32 kernel32 ntdll)
