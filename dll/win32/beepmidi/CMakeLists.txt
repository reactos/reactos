
spec2def(beepmidi.dll beepmidi.spec)

list(APPEND SOURCE
    beepmidi.c
    ${CMAKE_CURRENT_BINARY_DIR}/beepmidi.def)

add_library(beepmidi SHARED ${SOURCE})

set_module_type(beepmidi win32dll ENTRYPOINT 0 UNICODE)

add_importlibs(beepmidi winmm kernel32 ntdll msvcrt)


add_dependencies(beepmidi psdk)
add_cd_file(TARGET beepmidi DESTINATION reactos/system32 FOR all)
