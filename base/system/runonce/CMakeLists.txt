
add_rc_deps(runonce.rc ${CMAKE_CURRENT_SOURCE_DIR}/res/icon.ico)
add_executable(runonce runonce.c runonce.rc)
set_module_type(runonce win32gui UNICODE)
add_importlibs(runonce advapi32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET runonce DESTINATION reactos/system32 FOR all)
