
spec2def(sspicli.dll sspicli.spec)

list(APPEND SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${CMAKE_CURRENT_BINARY_DIR}/sspicli_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/sspicli.def)

add_library(sspicli MODULE ${SOURCE})
set_module_type(sspicli win32dll UNICODE)
add_importlibs(sspicli secur32 user32 kernel32 msvcrt ntdll)
target_link_libraries(sspicli wine oldnames)
set_wine_module(sspicli)
add_cd_file(TARGET sspicli DESTINATION reactos/system32 FOR all)
