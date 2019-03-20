
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(cryptdlg.dll cryptdlg.spec)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/cryptdlg_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/cryptdlg.def)

add_library(cryptdlg MODULE ${SOURCE} cryptdlg.rc)
set_module_type(cryptdlg win32dll)
target_link_libraries(cryptdlg wine)
add_importlibs(cryptdlg advapi32 user32 crypt32 cryptui wintrust msvcrt kernel32 ntdll)
add_cd_file(TARGET cryptdlg DESTINATION reactos/system32 FOR all)
