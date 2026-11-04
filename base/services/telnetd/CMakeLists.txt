set(SOURCE_FILES
    telnetd.cpp
    session.cpp
    session_handler.cpp
    tcpserver.cpp
    serviceentry.c
)

add_executable(telnetd
    ${SOURCE_FILES}
    telnetd.rc
)

target_link_libraries(telnetd wine cppstl)

set_module_type(telnetd win32cui)

add_importlibs(telnetd advapi32 ws2_32 msvcrt kernel32 ntdll userenv)

add_cd_file(TARGET telnetd DESTINATION reactos/system32 FOR all)
