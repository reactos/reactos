
list(APPEND SOURCE
    beepmidi.c)

add_library(beepmidi MODULE ${SOURCE})
spec2def(beepmidi beepmidi.dll beepmidi.spec)

set_module_type(beepmidi win32dll UNICODE)
add_importlibs(beepmidi winmm msvcrt kernel32 ntdll)
add_cd_file(TARGET beepmidi DESTINATION reactos/system32 FOR all)
