
add_definitions(-D__USE_W32_SOCKETS)
add_executable(client main.c)
set_module_type(client win32cui UNICODE)
add_importlibs(client user32 ws2_32 msvcrt kernel32)

if(MSVC)
    add_importlibs(client ntdll)
endif()

add_cd_file(TARGET client DESTINATION reactos/system32 FOR all)
