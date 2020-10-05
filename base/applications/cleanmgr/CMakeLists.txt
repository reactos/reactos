remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

list(APPEND SOURCE
    choicepage.c
    dialog.c
    main.c
    optionspage.c
    util.c
    stageflagpage.c
    precomp.h)

file(GLOB cleanmgr_rc_deps res/*.*)
add_rc_deps(resource.rc ${cleanmgr_rc_deps})
add_executable(cleanmgr ${SOURCE} resource.rc)
target_link_libraries(cleanmgr wine uuid)
set_module_type(cleanmgr win32gui UNICODE)
add_importlibs(cleanmgr user32 version comctl32 shell32 shlwapi gdi32 ole32 uxtheme advapi32 msvcrt kernel32 ntdll)
add_pch(cleanmgr precomp.h SOURCE)
add_cd_file(TARGET cleanmgr DESTINATION reactos/system32 FOR all)
