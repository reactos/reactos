
list(APPEND SOURCE
    about.c
    framewnd.c
    childwnd.c
    main.c)

add_executable(template_mdi ${SOURCE} mdi.rc)
set_module_type(template_mdi win32cui)
add_importlibs(template_mdi advapi32 user32 gdi32 comctl32 msvcrt kernel32 ntdll)
add_cd_file(TARGET template_mdi DESTINATION reactos/system32 FOR all)
