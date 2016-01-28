
add_definitions(-D__USE_W32_SOCKETS)
add_executable(ping ping.c ping.rc)
set_module_type(ping win32cui UNICODE)
add_importlibs(ping user32 ws2_32 msvcrt kernel32)

if(MSVC)
    add_importlibs(ping ntdll)
endif()

add_cd_file(TARGET ping DESTINATION reactos/system32 FOR all)
