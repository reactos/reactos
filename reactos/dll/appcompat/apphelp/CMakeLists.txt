
spec2def(apphelp.dll apphelp.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    apphelp.c
    layer.c
    sdbapi.c
    sdbfileattr.c
    apphelp.spec
    apphelp.h
    ${CMAKE_CURRENT_BINARY_DIR}/apphelp_stubs.c)

add_library(apphelp SHARED
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/apphelp.def)

set_module_type(apphelp win32dll)
target_link_libraries(apphelp wine)
# When binutils is fixed, we should move imagehlp to delay! CORE-6504
add_delay_importlibs(apphelp version)
add_importlibs(apphelp msvcrt imagehlp kernel32 ntdll)
add_cd_file(TARGET apphelp DESTINATION reactos/system32 FOR all)
