
project(SHELL)

add_definitions(
    -D__WINESRC__
    -D_SHLWAPI_)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(shlwapi.dll shlwapi.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    assoc.c
    clist.c
    istream.c
    msgbox.c
    ordinal.c
    path.c
    reg.c
    regstream.c
    shlwapi_main.c
    stopwatch.c
    string.c
    thread.c
    url.c
    precomp.h)

add_library(shlwapi SHARED
    ${SOURCE}
    shlwapi.rc
    wsprintf.c
    ${CMAKE_CURRENT_BINARY_DIR}/shlwapi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/shlwapi.def)

set_module_type(shlwapi win32dll)
target_link_libraries(shlwapi uuid wine)
add_delay_importlibs(shlwapi userenv oleaut32 ole32 comdlg32 mpr mlang urlmon shell32 winmm version)
add_importlibs(shlwapi user32 gdi32 advapi32 wininet msvcrt kernel32 ntdll)
add_pch(shlwapi precomp.h SOURCE)
add_cd_file(TARGET shlwapi DESTINATION reactos/system32 FOR all)
