
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(wbemdisp.dll wbemdisp.spec)

list(APPEND SOURCE
    locator.c
    main.c
    guid.c
    ${CMAKE_CURRENT_BINARY_DIR}/wbemdisp.def)

add_idl_headers(wbemdisp_idlheader wbemdisp_classes.idl)
add_typelib(wbemdisp_tlb.idl)

list(APPEND wbemdisp_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/wbemdisp_classes.rgs
    ${CMAKE_CURRENT_SOURCE_DIR}/wbemdisp_tlb.rgs
    ${CMAKE_CURRENT_BINARY_DIR}/wbemdisp_tlb.tlb)

set_source_files_properties(wbemdisp.rc PROPERTIES OBJECT_DEPENDS "${wbemdisp_rc_deps}")

add_library(wbemdisp SHARED ${SOURCE} wbemdisp.rc)
set_module_type(wbemdisp win32dll)
target_link_libraries(wbemdisp uuid wine)
add_dependencies(wbemdisp wbemdisp_idlheader)
add_importlibs(wbemdisp oleaut32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wbemdisp DESTINATION reactos/system32/wbem FOR all)
