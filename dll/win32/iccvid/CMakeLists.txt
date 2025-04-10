
spec2def(iccvid.dll iccvid.spec)

list(APPEND SOURCE
    iccvid.c
    ${CMAKE_CURRENT_BINARY_DIR}/iccvid.def)

add_library(iccvid MODULE ${SOURCE} rsrc.rc)
set_module_type(iccvid win32dll)
target_link_libraries(iccvid wine)
add_importlibs(iccvid user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET iccvid DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(iccvid) # CORE-5743: No ARRAY_SIZE macro
