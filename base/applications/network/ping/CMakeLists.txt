add_definitions(-D__USE_W32_SOCKETS)

add_executable(ping ping.c ping.rc)

set_module_type(ping win32cui)

add_importlibs(ping ws2_32 msvcrt)
