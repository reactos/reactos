set_unicode(audclient yes)



add_executable(audclient
    audclient.c)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/audsrv)
set_module_type(audclient win32cui)
add_importlibs(audclient audsrvapi advapi32 user32 ksuser setupapi msvcrt kernel32)
add_cab_target(audclient 1)
