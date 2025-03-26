
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -D_WINE
    -DLIBXML_STATIC
    -DCOM_NO_WINDOWS_H)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/libwin-iconv)

spec2def(msxml3.dll msxml3.spec ADD_IMPORTLIB NO_PRIVATE_WARNINGS)
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
    xdr.c
    xmldoc.c
    xmlelem.c
    xmlparser.c
    xmlview.c)

FLEX_TARGET(xslpattern_scanner xslpattern.l ${CMAKE_CURRENT_BINARY_DIR}/xslpattern.yy.c)
BISON_TARGET(xslpattern_parser xslpattern.y ${CMAKE_CURRENT_BINARY_DIR}/xslpattern.tab.c COMPILE_FLAGS "-p xslpattern_")
ADD_FLEX_BISON_DEPENDENCY(xslpattern_scanner xslpattern_parser)

list(APPEND PCH_SKIP_SOURCE
    factory.c
    uuid.c
    ${FLEX_xslpattern_scanner_OUTPUTS}
    ${BISON_xslpattern_parser_OUTPUTS})

list(APPEND msxml3_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/msxml3.manifest
    ${CMAKE_CURRENT_SOURCE_DIR}/msxml3_v1.rgs
    ${CMAKE_CURRENT_SOURCE_DIR}/xmlparser.rgs
    ${CMAKE_CURRENT_BINARY_DIR}/msxml3_v1.tlb)

set_source_files_properties(version.rc PROPERTIES OBJECT_DEPENDS "${msxml3_rc_deps}")

add_library(msxml3 MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msxml3_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msxml3.def)

add_idl_headers(xmlparser_idlheader xmlparser.idl)
set_module_type(msxml3 win32dll)
target_link_libraries(msxml3 libxml2 iconv-static uuid wine zlib)

if(MSVC)
    target_compile_options(msxml3 PRIVATE /FIwine/typeof.h /FImsvc.h)

    # Disable warning C4090: 'function': different 'const' qualifiers
    target_compile_options(msxml3 PRIVATE /wd4090)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(msxml3 PRIVATE -Wno-incompatible-function-pointer-types)
endif()

add_importlibs(msxml3 urlmon ws2_32 shlwapi oleaut32 ole32 user32 msvcrt kernel32 ntdll)
add_dependencies(msxml3 xmlparser_idlheader stdole2) # msxml3_v1.tlb needs stdole2.tlb
add_pch(msxml3 precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET msxml3 DESTINATION reactos/system32 FOR all)
