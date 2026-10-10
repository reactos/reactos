
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(objsel.dll objsel.spec)

add_library(objsel MODULE
    objsel.c
    objsel.rc
    ${CMAKE_CURRENT_BINARY_DIR}/objsel.def)

set_module_type(objsel win32dll)
add_idl_reg_scripts(objsel registry objsel_classes.idl)
target_link_libraries(objsel uuid wine wine_dll_register wine_dll_canunload)
add_importlibs(objsel ole32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET objsel DESTINATION reactos/system32 FOR all)
set_wine_module(objsel)
