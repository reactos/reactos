
spec2def(kbsdll.dll kbsdll.spec)

list(APPEND SOURCE
    kbsdll.c
    kbsdll.rc
    ${CMAKE_CURRENT_BINARY_DIR}/kbsdll.def)

add_library(kbsdll SHARED ${SOURCE})
set_module_type(kbsdll win32dll UNICODE)
add_importlibs(kbsdll user32 comctl32 msvcrt kernel32)
add_cd_file(TARGET kbsdll DESTINATION reactos/system32 FOR all)
