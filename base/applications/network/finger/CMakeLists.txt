add_definitions(-D__USE_W32_SOCKETS)
add_definitions(-D_DLL -D__USE_CRTIMP)

add_executable(finger 
    finger.c
    err.c
    getopt.c
    net.c
    finger.rc)

set_module_type(finger win32cui)

add_importlibs(finger ws2_32 msvcrt kernel32)

add_cab_target(finger 1)