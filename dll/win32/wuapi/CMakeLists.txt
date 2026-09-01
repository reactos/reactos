
add_definitions(
    -DCOM_NO_WINDOWS_H)

spec2def(wuapi.dll wuapi.spec)

list(APPEND SOURCE
    downloader.c
    installer.c
    main.c
    searcher.c
    session.c
    systeminfo.c
    updates.c
    precomp.h)

add_library(wuapi MODULE
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/wuapi.def)

set_module_type(wuapi win32dll)
add_idl_reg_scripts(wuapi registry wuapi_tlb.idl)
target_link_libraries(wuapi uuid wine wine_dll_register wine_dll_canunload wuguid oldnames)
add_importlibs(wuapi msvcrt kernel32 ntdll)
add_dependencies(wuapi stdole2) # wuapi_tlb.tlb needs stdole2.tlb
add_pch(wuapi precomp.h SOURCE)
add_cd_file(TARGET wuapi DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(wuapi) # CORE-5743: No CONST_VTABLE
