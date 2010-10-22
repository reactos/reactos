
add_definitions(-D__WINESRC__)
add_definitions(-D_WINE)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(cryptui ${CMAKE_CURRENT_SOURCE_DIR}/cryptui.spec ${CMAKE_CURRENT_BINARY_DIR}/cryptui.def)

add_library(cryptui SHARED main.c cryptui.rc)

set_module_type(cryptui win32dll)

target_link_libraries(cryptui
    ${CMAKE_CURRENT_BINARY_DIR}/cryptui.def
    uuid
    wine)

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
    kernel32
    ntdll)

add_dependencies(cryptui cryptui_def)
add_livecd_target(cryptui reactos/system32)
