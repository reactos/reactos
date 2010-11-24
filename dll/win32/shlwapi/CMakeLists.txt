
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(shlwapi.dll shlwapi.spec)

add_library(shlwapi SHARED
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
    wsprintf.c
    shlwapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shlwapi.def)

set_module_type(shlwapi win32dll)

target_link_libraries(shlwapi
    wine
    uuid)

add_importlibs(shlwapi
    user32
    gdi32
    advapi32
    oleaut32
    ole32
    comctl32
    comdlg32
    mpr
    mlang
    urlmon
    winmm
    version
    shell32
    kernel32
    msvcrt
    ntdll)


add_cab_target(shlwapi 1)
add_importlib_target(shlwapi.spec)
