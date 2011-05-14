
add_definitions(
    -D__USE_W32_SOCKETS
    -D_DLL -D__USE_CRTIMP)

add_executable(finger 
    finger.c
    err.c
    getopt.c
    net.c
    finger.rc)

set_module_type(finger win32cui)
add_importlibs(finger ws2_32 msvcrt kernel32)
add_cd_file(TARGET finger DESTINATION reactos/system32)
