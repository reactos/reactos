set_unicode(audiosrv2exe yes)



add_executable(audiosrv2exe
    audiosrv.c)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/audiosrv2)
set_module_type(audiosrv2exe win32cui)
add_importlibs(audiosrv2exe audiosrv2dll advapi32 user32 ksuser setupapi msvcrt kernel32)
add_cab_target(audiosrv2exe 1)
