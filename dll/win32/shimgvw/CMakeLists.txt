
list(APPEND SOURCE
    shimgvw.c
    comsup.c
    shimgvw.rc)

add_library(shimgvw MODULE ${SOURCE})
spec2def(shimgvw shimgvw.dll shimgvw.spec)

set_module_type(shimgvw win32dll)
target_link_libraries(shimgvw wine)
add_importlibs(shimgvw advapi32 comctl32 user32 gdi32 gdiplus comdlg32 shlwapi msvcrt kernel32 ntdll)
add_cd_file(TARGET shimgvw DESTINATION reactos/system32 FOR all)
