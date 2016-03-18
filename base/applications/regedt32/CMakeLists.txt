
add_rc_deps(resource.rc ${CMAKE_CURRENT_SOURCE_DIR}/res/regedt32.ico)
add_executable(regedt32 regedt32.c resource.rc)
set_module_type(regedt32 win32gui UNICODE)
add_importlibs(regedt32 shell32 shlwapi msvcrt kernel32)
add_cd_file(TARGET regedt32 DESTINATION reactos/system32 FOR all)
