
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(cryptdlg.dll cryptdlg.spec)

add_library(cryptdlg SHARED main.c cryptdlg.rc ${CMAKE_CURRENT_BINARY_DIR}/cryptdlg.def)

set_module_type(cryptdlg win32dll)

target_link_libraries(cryptdlg wine)

add_importlibs(cryptdlg
    advapi32
    user32
    crypt32
    cryptui
    wintrust
    msvcrt
    kernel32
    ntdll)


add_cab_target(cryptdlg 1)
