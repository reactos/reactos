
spec2def(umandlg.dll umandlg.spec)

add_library(umandlg MODULE
    about.c
    process.c
    registry.c
    umandlg.c
    umandlg.h
    umandlg.rc
    ${CMAKE_CURRENT_BINARY_DIR}/umandlg.def)

set_module_type(umandlg win32dll UNICODE)
add_importlibs(umandlg comdlg32 user32 gdi32 advapi32 shell32 comctl32 msvcrt kernel32 ntdll)
add_cd_file(TARGET umandlg DESTINATION reactos/system32 FOR all)
