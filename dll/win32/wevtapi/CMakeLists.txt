
spec2def(wevtapi.dll wevtapi.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/wevtapi_stubs.c)

add_library(wevtapi MODULE
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/wevtapi.def)

set_module_type(wevtapi win32dll)
target_link_libraries(wevtapi wine wine_dll_canunload)
add_importlibs(wevtapi advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wevtapi DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(wevtapi) # CORE-5743: No ARRAY_SIZE macro
