
add_rc_deps(resource.rc ${CMAKE_CURRENT_SOURCE_DIR}/res/setup.ico)
add_executable(wusa wusa.c resource.rc)
set_module_type(wusa win32gui UNICODE)
add_importlibs(wusa shell32 msvcrt kernel32)
add_cd_file(TARGET wusa DESTINATION reactos/system32 FOR all)
