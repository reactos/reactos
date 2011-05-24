
set_unicode()

add_definitions(-DWIN32)

set_rc_compiler()

list(APPEND SOURCE
    desktop.c
    dragdrop.c
    explorer.c
    startmnu.c
    taskband.c
    taskswnd.c
    tbsite.c
    trayntfy.c
    trayprop.c
    traywnd.c
    explorer.rc)

add_executable(explorer_new ${CMAKE_CURRENT_BINARY_DIR}/explorer_new_precomp.h.gch ${SOURCE})

target_link_libraries(explorer_new uuid)

add_pch(explorer_new ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
set_module_type(explorer_new win32gui)
add_importlibs(explorer_new
    advapi32
    gdi32
    user32
    comctl32
    ole32
    oleaut32
    shell32
    shlwapi
    version
    msvcrt
    kernel32
    ntdll)

add_cd_file(TARGET explorer_new DESTINATION reactos FOR all)
