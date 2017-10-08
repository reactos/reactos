
add_definitions(-D__USE_W32_SOCKETS)
add_executable(tracert tracert.c tracert.rc)
set_module_type(tracert win32cui)
add_importlibs(tracert ws2_32 msvcrt kernel32)

if(MSVC)
    add_importlibs(tracert ntdll)
endif()

add_cd_file(TARGET tracert DESTINATION reactos/system32 FOR all)
