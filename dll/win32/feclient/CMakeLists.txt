
spec2def(feclient.dll feclient.spec)

list(APPEND SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/feclient_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/feclient.def)

add_library(feclient MODULE ${SOURCE})
set_module_type(feclient win32dll UNICODE ENTRYPOINT 0)
add_importlibs(feclient user32 kernel32 ntdll)
target_link_libraries(feclient wine)
set_wine_module(feclient)

add_cd_file(TARGET feclient DESTINATION reactos/system32 FOR all)
