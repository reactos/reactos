
add_executable(doskey doskey.c doskey.rc)
set_module_type(doskey win32cui UNICODE)
add_importlibs(doskey user32 msvcrt kernel32)
add_cd_file(TARGET doskey DESTINATION reactos/system32 FOR all)
