add_definitions(-D_DLL -D__USE_CRTIMP)

add_executable(hostname hostname.c hostname.rc)

set_module_type(hostname win32cui)
add_importlibs(hostname msvcrt kernel32)

add_cab_target(hostname 1)