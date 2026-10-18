
spec2def(bcp47langs.dll bcp47langs.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/bcp47langs_stubs.c)

add_library(bcp47langs MODULE
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/bcp47langs.def)

set_module_type(bcp47langs win32dll)
target_link_libraries(bcp47langs wine wine_dll_canunload)
add_importlibs(bcp47langs advapi32 msvcrt kernel32 kernel32_vista ntdll)
add_cd_file(TARGET bcp47langs DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(bcp47langs) # CORE-5743: No ARRAY_SIZE macro
