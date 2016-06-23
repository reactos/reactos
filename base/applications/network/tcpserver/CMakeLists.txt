
add_definitions(-D__USE_W32_SOCKETS)
add_executable(server main.c)
set_module_type(server win32cui UNICODE)
add_importlibs(server user32 ws2_32 msvcrt kernel32)

if(MSVC)
    add_importlibs(server ntdll)
endif()

add_cd_file(TARGET server DESTINATION reactos/system32 FOR all)
