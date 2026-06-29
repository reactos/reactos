
spec2def(msvcrt20.dll msvcrt20.spec)

add_library(msvcrt20 MODULE
    msvcrt20.c
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt20_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt20.def)

set_module_type(msvcrt20 win32dll ENTRYPOINT DllMain 12)
add_dependencies(msvcrt20 psdk)
target_link_libraries(msvcrt20 msvcrt_static crtmath vcruntime wine chkstk ${PSEH_LIB})

add_importlibs(msvcrt20 kernel32 kernel32_vista ntdll)
add_delay_importlibs(msvcrt20 user32 advapi32)

add_cd_file(TARGET msvcrt20 DESTINATION reactos/system32 FOR all)
