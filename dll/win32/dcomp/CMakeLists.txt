
spec2def(dcomp.dll dcomp.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    device.c
    ${CMAKE_CURRENT_BINARY_DIR}/dcomp_stubs.c)

add_library(dcomp MODULE
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/dcomp.def)

set_module_type(dcomp win32dll)
target_link_libraries(dcomp wine wine_dll_canunload)
add_importlibs(dcomp advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET dcomp DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(dcomp) # CORE-5743: No ARRAY_SIZE macro
