
add_executable(ipconfig ipconfig.c ipconfig.rc)

set_module_type(ipconfig win32cui)

add_importlibs(ipconfig user32 iphlpapi advapi32 msvcrt)
