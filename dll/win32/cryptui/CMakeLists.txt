
add_definitions(
    -D__WINESRC__
    -D_WINE)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()

spec2def(cryptui.dll cryptui.spec)

list(APPEND SOURCE
    main.c
    cryptui.rc
    ${CMAKE_CURRENT_BINARY_DIR}/cryptui_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/cryptui.def)

add_library(cryptui SHARED ${SOURCE})

set_module_type(cryptui win32dll)

target_link_libraries(cryptui uuid wine)

add_importlibs(cryptui
    user32
    ole32
    crypt32
    gdi32
    advapi32
    urlmon
    wintrust
    comctl32
    comdlg32
    msvcrt
    kernel32
    ntdll)

add_importlib_target(cryptui.spec)
add_cd_file(TARGET cryptui DESTINATION reactos/system32 FOR all)
