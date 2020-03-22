
spec2def(storprop.dll storprop.spec)

add_library(storprop MODULE
    storprop.c
    storprop.rc
    ${CMAKE_CURRENT_BINARY_DIR}/storprop_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/storprop.def)

set_module_type(storprop win32dll UNICODE)
target_link_libraries(storprop wine)
add_importlibs(storprop setupapi advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET storprop DESTINATION reactos/system32 FOR all)
