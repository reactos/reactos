
set_unicode()

add_executable(regedt32 regedt32.c resource.rc)

set_module_type(regedt32 win32gui)
add_importlibs(regedt32 shell32 shlwapi msvcrt kernel32)
add_cab_target(regedt32 1)
