
add_definitions(
    -D__WINESRC__
    -D_WINE
    -DLIBXML_STATIC
    -DCOM_NO_WINDOWS_H)

if(MSVC)
    add_compile_flags("/FIwine/typeof.h /FImsvc.h")
endif()

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(msxml3.dll msxml3.spec ADD_IMPORTLIB)
add_typelib(msxml3_v1.idl)

list(APPEND SOURCE
    attribute.c
    bsc.c
    cdata.c
    comment.c
    dispex.c
    docfrag.c
    doctype.c
    domdoc.c
    domimpl.c
    element.c
    entityref.c
    factory.c
    httprequest.c
    main.c
    mxnamespace.c
    mxwriter.c
    node.c
    nodelist.c
    nodemap.c
    parseerror.c
    pi.c
    saxreader.c
    schema.c
    selection.c
    stylesheet.c
    text.c
    uuid.c
    xdr.c
    xmldoc.c
    xmlelem.c
    xmlparser.c
    xmlview.c
    xslpattern.tab.c
    xslpattern.yy.c
    ${CMAKE_CURRENT_BINARY_DIR}/msxml3_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msxml3.def)

list(APPEND msxml3_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/msxml3.manifest
    ${CMAKE_CURRENT_SOURCE_DIR}/msxml3_v1.rgs
    ${CMAKE_CURRENT_SOURCE_DIR}/xmlparser.rgs
    ${CMAKE_CURRENT_BINARY_DIR}/msxml3_v1.tlb)

set_source_files_properties(version.rc PROPERTIES OBJECT_DEPENDS "${msxml3_rc_deps}")
add_library(msxml3 SHARED ${SOURCE} version.rc)
add_idl_headers(xmlparser_idlheader xmlparser.idl)
set_module_type(msxml3 win32dll)
target_link_libraries(msxml3 libxml2 uuid wine)
add_importlibs(msxml3 urlmon ws2_32 shlwapi oleaut32 ole32 user32 msvcrt kernel32 ntdll)
add_dependencies(msxml3 xmlparser_idlheader stdole2) # msxml3_v1.tlb needs stdole2.tlb
add_cd_file(TARGET msxml3 DESTINATION reactos/system32 FOR all)
