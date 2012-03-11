
set_rc_compiler()

add_executable(setup16
    main.c)

set_module_type(setup16 win32gui UNICODE)
add_importlibs(setup16 user32 gdi32 advapi32 msvcrt kernel32 shell32 setupapi)

add_cd_file(TARGET setup16 DESTINATION reactos/system32 FOR all)
