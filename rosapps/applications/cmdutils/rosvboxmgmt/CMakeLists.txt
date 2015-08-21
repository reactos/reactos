add_executable(rosvboxmgmt rosvboxmgmt.c rosvboxmgmt.rc)
set_module_type(rosvboxmgmt win32cui UNICODE)
add_importlibs(rosvboxmgmt msvcrt kernel32)
add_cd_file(TARGET rosvboxmgmt DESTINATION reactos/bin FOR all)
