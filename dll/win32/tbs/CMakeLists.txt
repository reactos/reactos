
spec2def(tbs.dll tbs.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    tbs.c
    ${CMAKE_CURRENT_BINARY_DIR}/tbs_stubs.c)

add_library(tbs MODULE
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/tbs.def)

set_module_type(tbs win32dll)
target_link_libraries(tbs wine wine_dll_canunload)
add_importlibs(tbs advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET tbs DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(tbs) # CORE-5743: No ARRAY_SIZE macro
