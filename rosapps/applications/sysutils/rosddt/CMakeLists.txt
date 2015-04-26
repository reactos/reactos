
add_executable(rosddt http.c rosddt.c rosddt.rc)
set_module_type(rosddt win32cui UNICODE)
add_importlibs(rosddt setupapi wininet user32 msvcrt kernel32)
add_cd_file(TARGET rosddt DESTINATION reactos/system32 FOR all)
