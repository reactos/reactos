
list(APPEND SOURCE
    ntlanman.c
    ntlanman.rc)

add_library(ntlanman MODULE ${SOURCE})
spec2def(ntlanman ntlanman.dll ntlanman.spec)

set_module_type(ntlanman win32dll UNICODE)
target_link_libraries(ntlanman wine)
add_importlibs(ntlanman netapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET ntlanman DESTINATION reactos/system32 FOR all)
