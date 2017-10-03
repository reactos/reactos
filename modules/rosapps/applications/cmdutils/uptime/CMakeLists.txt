#add_definitions(-DLINUX_OUTPUT)

add_executable(uptime uptime.c uptime.rc)
set_module_type(uptime win32cui)
add_importlibs(uptime msvcrt kernel32)
if(MSVC)
    add_importlibs(uptime ntdll)
endif()
add_cd_file(TARGET uptime DESTINATION reactos/system32 FOR all)
