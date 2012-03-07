
add_executable(installfreeldr EXCLUDE_FROM_ALL
    install.c
    volume.c)

set_module_type(installfreeldr win32cui)

add_importlibs(installfreeldr msvcrt kernel32)
