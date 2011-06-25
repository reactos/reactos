
set_unicode()

spec2def(beepmidi.dll beepmidi.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    beepmidi.c
    ${CMAKE_CURRENT_BINARY_DIR}/beepmidi.def)

add_library(beepmidi SHARED ${SOURCE})

set_entrypoint(beepmidi 0)
add_importlibs(beepmidi winmm kernel32 ntdll msvcrt)
add_importlib_target(beepmidi.spec)

add_dependencies(beepmidi psdk)
add_cd_file(TARGET beepmidi DESTINATION reactos/system32 FOR all)
