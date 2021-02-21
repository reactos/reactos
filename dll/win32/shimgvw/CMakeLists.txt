
spec2def(shimgvw.dll shimgvw.spec)

list(APPEND SOURCE
    shimgvw.c
    comsup.c
    shimgvw.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shimgvw_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/shimgvw.def)

add_library(shimgvw MODULE ${SOURCE})
set_module_type(shimgvw win32dll UNICODE)
target_link_libraries(shimgvw wine)
add_importlibs(shimgvw advapi32 comctl32 user32 gdi32 shell32 ole32 gdiplus comdlg32 shlwapi msvcrt kernel32 ntdll)
add_cd_file(TARGET shimgvw DESTINATION reactos/system32 FOR all)
