
add_library(clb MODULE
    clb.c
    clb.rc)
spec2def(clb clb.dll clb.spec)

set_module_type(clb win32dll UNICODE)
add_importlibs(clb user32 comctl32 msvcrt kernel32 ntdll)
