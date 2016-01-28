
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(wbemdisp.dll wbemdisp.spec)

list(APPEND SOURCE
    locator.c
    main.c
    wbemdisp_private.h)

add_idl_headers(wbemdisp_idlheader wbemdisp_classes.idl)
add_typelib(wbemdisp_tlb.idl)

list(APPEND wbemdisp_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/wbemdisp_classes.rgs
    ${CMAKE_CURRENT_SOURCE_DIR}/wbemdisp_tlb.rgs
    ${CMAKE_CURRENT_BINARY_DIR}/wbemdisp_tlb.tlb)

set_source_files_properties(wbemdisp.rc PROPERTIES OBJECT_DEPENDS "${wbemdisp_rc_deps}")

add_library(wbemdisp SHARED
    ${SOURCE}
    guid.c
    wbemdisp.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wbemdisp.def)

set_module_type(wbemdisp win32dll)
target_link_libraries(wbemdisp uuid wine)
add_dependencies(wbemdisp stdole2 wbemdisp_idlheader)
add_importlibs(wbemdisp oleaut32 ole32 msvcrt kernel32 ntdll)
add_pch(wbemdisp wbemdisp_private.h SOURCE)
add_cd_file(TARGET wbemdisp DESTINATION reactos/system32/wbem FOR all)
