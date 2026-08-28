
spec2def(initpki.dll initpki.spec)

list(APPEND SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/initpki_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/initpki.def)

add_library(initpki MODULE ${SOURCE})
set_module_type(initpki win32dll)
target_link_libraries(initpki wine wine_dll_register wine_dll_canunload)
add_importlibs(initpki msvcrt kernel32 ntdll)
add_cd_file(TARGET initpki DESTINATION reactos/system32 FOR all)
set_wine_module(initpki)
