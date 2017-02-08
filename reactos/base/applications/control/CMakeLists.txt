
add_rc_deps(control.rc ${CMAKE_CURRENT_SOURCE_DIR}/resources/config.ico)
add_executable(control control.c control.rc)
set_module_type(control win32gui UNICODE)
add_importlibs(control advapi32 shell32 user32 msvcrt kernel32)
add_cd_file(TARGET control DESTINATION reactos/system32 FOR all)
