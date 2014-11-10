
add_executable(winver winver.c winver.rc)
set_module_type(winver win32gui UNICODE)
add_importlibs(winver shell32 msvcrt kernel32)
add_cd_file(TARGET winver DESTINATION reactos/system32 FOR all)
