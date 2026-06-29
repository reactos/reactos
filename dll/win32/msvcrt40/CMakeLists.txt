
spec2def(msvcrt40.dll msvcrt40.spec)

add_library(msvcrt40 MODULE
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt40_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt40.def)

set_module_type(msvcrt40 win32dll ENTRYPOINT DllMain 12)
add_dependencies(msvcrt40 psdk)
target_link_libraries(msvcrt40 msvcrt_static crtmath vcruntime chkstk wine ${PSEH_LIB})

add_importlibs(msvcrt40 kernel32 kernel32_vista ntdll)
add_delay_importlibs(msvcrt40 user32 advapi32)
add_cd_file(TARGET msvcrt40 DESTINATION reactos/system32 FOR all)
