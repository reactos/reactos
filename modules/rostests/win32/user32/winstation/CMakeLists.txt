
add_executable(winstation winstation.c winstation.rc)
set_module_type(winstation win32gui UNICODE)
add_importlibs(winstation user32 msvcrt kernel32)
add_rostests_file(TARGET winstation SUBDIR suppl)
