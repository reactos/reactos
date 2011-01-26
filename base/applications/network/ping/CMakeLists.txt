
set_unicode()
add_definitions(-D__USE_W32_SOCKETS)

set_unicode()

add_executable(ping ping.c ping.rc)

set_module_type(ping win32cui)

add_importlibs(ping ws2_32 msvcrt kernel32)
if(MSVC)
add_importlibs(ping ntdll)
endif()

add_cab_target(ping 1)
