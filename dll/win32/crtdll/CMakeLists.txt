
spec2def(crtdll.dll crtdll.spec ADD_IMPORTLIB)

add_definitions(
    -DUSE_MSVCRT_PREFIX
    -DCRTDLL)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/crt/include)

list(APPEND SOURCE
    dllmain.c
    crtdll.rc
    ${CMAKE_CURRENT_BINARY_DIR}/crtdll_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/crtdll.def)

add_library(crtdll MODULE ${SOURCE})
set_module_type(crtdll win32dll ENTRYPOINT DllMain 12)
target_link_libraries(crtdll msvcrt_static crtmath vcruntime chkstk wine ${PSEH_LIB})
add_dependencies(crtdll psdk)

add_importlibs(crtdll kernel32 kernel32_vista ntdll)
add_delay_importlibs(crtdll user32)
add_cd_file(TARGET crtdll DESTINATION reactos/system32 FOR all)
