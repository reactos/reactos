
set_unicode()
set_rc_compiler()

list(APPEND SOURCE cacls.c cacls.rc)

add_executable(cacls ${SOURCE})

set_module_type(cacls win32cui)
add_importlibs(cacls advapi32 user32 shell32 msvcrt kernel32)
add_cd_file(TARGET cacls DESTINATION reactos/system32 FOR all)
