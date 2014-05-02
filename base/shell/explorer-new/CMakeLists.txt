
add_definitions(-DWIN32)

list(APPEND SOURCE
    desktop.c
    dragdrop.c
    explorer.c
    settings.c
    startmnu.c
    startup.c
    taskband.c
    taskswnd.c
    tbsite.c
    trayntfy.c
    trayprop.c
    traywnd.c
    precomp.h)

add_executable(explorer_new ${SOURCE} explorer.rc)
target_link_libraries(explorer_new uuid)
set_module_type(explorer_new win32gui UNICODE)
add_importlibs(explorer_new
    advapi32
    gdi32
    user32
    comctl32
    ole32
    oleaut32
    shell32
    shlwapi
    shdocvw
    version
    uxtheme
    msvcrt
    kernel32
    ntdll)
add_pch(explorer_new precomp.h SOURCE)
add_cd_file(TARGET explorer_new DESTINATION reactos FOR all)
