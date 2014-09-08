
PROJECT(SHELL)

add_definitions(-DWIN32)

list(APPEND SOURCE
    desktop.c
    dragdrop.c
    explorer.c
    rshell.c
    settings.c
    shellservice.c
    startmnu.c
    startup.c
    taskband.c
    taskswnd.c
    tbsite.c
    trayntfy.c
    trayprop.c
    traywnd.c
    precomp.h)

add_executable(explorer ${SOURCE} explorer.rc)
target_link_libraries(explorer uuid wine)
set_module_type(explorer win32gui UNICODE)
add_importlibs(explorer
    advapi32
    gdi32
    user32
    comctl32
    ole32
    oleaut32
    shell32
    browseui
    shlwapi
    shdocvw
    version
    uxtheme
    msvcrt
    kernel32
    ntdll)
add_pch(explorer precomp.h SOURCE)
add_cd_file(TARGET explorer DESTINATION reactos FOR all)
