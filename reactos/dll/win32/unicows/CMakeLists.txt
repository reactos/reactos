
spec2def(unicows.dll unicows.spec)

list(APPEND SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/unicows_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/unicows.def)

add_library(unicows SHARED ${SOURCE})

set_module_type(unicows win32dll)

add_importlibs(unicows
    avicap32
    msvfw32
    oledlg
    comdlg32
    shell32
    winmm
    winspool
    rasapi32
    secur32
    sensapi
    version
    user32
    mpr
    gdi32
    advapi32
    msvcrt
    kernel32
    ntdll)
add_cd_file(TARGET unicows DESTINATION reactos FOR all)
