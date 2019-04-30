
project(appcompat)

add_executable(shimtest_ros shimtest_ros.c shimtest_ros.rc)

set_module_type(shimtest_ros win32cui)
add_importlibs(shimtest_ros shlwapi msvcrt kernel32 ntdll)
add_cd_file(TARGET shimtest_ros DESTINATION reactos/bin FOR all)
