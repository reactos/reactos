
spec2def(wmiutils.dll wmiutils.spec)

list(APPEND SOURCE
    main.c
    path.c
    statuscode.c
    ${CMAKE_CURRENT_BINARY_DIR}/wmiutils.def)

add_library(wmiutils MODULE ${SOURCE})
set_module_type(wmiutils win32dll)
add_idl_reg_scripts(wmiutils registry wmiutils_classes.idl)
target_link_libraries(wmiutils wine wine_dll_register wine_dll_canunload oldnames)
add_importlibs(wmiutils oleaut32 msvcrt kernel32 kernel32_vista ntdll)
add_cd_file(TARGET wmiutils DESTINATION reactos/system32/wbem FOR all)
set_wine_module_FIXME(wmiutils) # CORE-5743: No CONST_VTABLE
