
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    main.c)

add_library(cryptdlg MODULE ${SOURCE} cryptdlg.rc)
spec2def(cryptdlg cryptdlg.dll cryptdlg.spec)

set_module_type(cryptdlg win32dll)
target_link_libraries(cryptdlg wine)
add_importlibs(cryptdlg advapi32 user32 crypt32 cryptui wintrust msvcrt kernel32 ntdll)
add_cd_file(TARGET cryptdlg DESTINATION reactos/system32 FOR all)
