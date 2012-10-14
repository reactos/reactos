
set_rc_compiler()

add_executable(kbswitch kbswitch.c kbswitch.rc)

set_module_type(kbswitch win32gui UNICODE)
add_importlibs(kbswitch advapi32 user32 shell32 gdi32 msvcrt kernel32)

add_cd_file(TARGET kbswitch DESTINATION reactos/system32 FOR all)
add_subdirectory(kbsdll)
