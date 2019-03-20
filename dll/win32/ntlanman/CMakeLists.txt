
spec2def(ntlanman.dll ntlanman.spec)

list(APPEND SOURCE
    ntlanman.c
    ntlanman.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ntlanman_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/ntlanman.def)

add_library(ntlanman MODULE ${SOURCE})
set_module_type(ntlanman win32dll UNICODE)
target_link_libraries(ntlanman wine)
add_importlibs(ntlanman netapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET ntlanman DESTINATION reactos/system32 FOR all)
