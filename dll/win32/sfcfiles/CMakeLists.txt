spec2def(sfcfiles.dll sfcfiles.spec)

list(APPEND SOURCE
    sfcfiles.c
    ${CMAKE_CURRENT_BINARY_DIR}/sfcfiles.def)

add_library(sfcfiles MODULE ${SOURCE})
set_module_type(sfcfiles win32dll)
add_importlibs(sfcfiles msvcrt kernel32 ntdll)
add_cd_file(TARGET sfcfiles DESTINATION reactos/system32 FOR all)
