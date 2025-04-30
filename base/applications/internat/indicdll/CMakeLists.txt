
spec2def(indicdll.dll indicdll.spec)

list(APPEND SOURCE
    indicdll.c
    indicdll.rc
    ${CMAKE_CURRENT_BINARY_DIR}/indicdll.def)

add_library(indicdll MODULE ${SOURCE})
set_module_type(indicdll win32dll UNICODE)
add_importlibs(indicdll user32 comctl32 msvcrt kernel32)
add_cd_file(TARGET indicdll DESTINATION reactos/system32 FOR all)
