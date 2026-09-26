
spec2def(wbemdisp.dll wbemdisp.spec)

list(APPEND SOURCE
    locator.c
    main.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_idl_headers(wbemdisp_idlheader wbemdisp_classes.idl)

add_library(wbemdisp MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/wbemdisp.def)

set_module_type(wbemdisp win32dll)
add_idl_reg_scripts(wbemdisp registry wbemdisp_classes.idl)
add_idl_reg_scripts(wbemdisp regtypelib wbemdisp_tlb.idl)
target_link_libraries(wbemdisp uuid wine wine_dll_register wine_dll_canunload oldnames)
add_dependencies(wbemdisp stdole2 wbemdisp_idlheader)
add_importlibs(wbemdisp oleaut32 ole32 msvcrt kernel32 ntdll)
add_pch(wbemdisp precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET wbemdisp DESTINATION reactos/system32/wbem FOR all)
set_wine_module_FIXME(wbemdisp) # CORE-5743: No ARRAY_SIZE and CONST_VTABLE
