
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(cryptdlg ${CMAKE_CURRENT_SOURCE_DIR}/cryptdlg.spec ${CMAKE_CURRENT_BINARY_DIR}/cryptdlg.def)

add_library(cryptdlg SHARED main.c cryptdlg.rc)

set_module_type(cryptdlg win32dll)

target_link_libraries(cryptdlg
    ${CMAKE_CURRENT_BINARY_DIR}/cryptdlg.def
    wine)

add_importlibs(cryptdlg
    advapi32
    user32
    crypt32
    cryptui
    wintrust
    kernel32
    ntdll)

add_dependencies(cryptdlg cryptdlg_def)
add_livecd_target(cryptdlg reactos/system32)
