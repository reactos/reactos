
list(APPEND SOURCE
    main.c
    dialog.c
    main.h)

file(GLOB winmine_rc_deps rc/*.*)
add_rc_deps(rsrc.rc ${winmine_rc_deps})
add_executable(winmine ${SOURCE} rsrc.rc)
add_pch(winmine main.h SOURCE)
set_module_type(winmine win32gui UNICODE)
target_link_libraries(winmine wine)
add_importlibs(winmine user32 gdi32 advapi32 shell32 winmm msvcrt kernel32 ntdll)
add_cd_file(TARGET winmine DESTINATION reactos/system32 FOR all)
