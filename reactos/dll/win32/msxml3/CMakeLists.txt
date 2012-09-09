
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x601)

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

add_library(msxml3 SHARED ${SOURCE} version.rc)

if(NOT MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #allow_warnings(msxml3)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-error")
endif()

add_idl_headers(xmlparser_idlheader xmlparser.idl)
add_dependencies(msxml3 xmlparser_idlheader)

set_module_type(msxml3 win32dll)
set_source_files_properties(version.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/msxml3_v1.tlb)
target_link_libraries(msxml3 libxml2 uuid wine wineldr)

add_importlibs(msxml3 urlmon wininet ws2_32 comctl32 shell32 shlwapi cabinet oleaut32 ole32 version user32 gdi32 advapi32 msvcrt kernel32 ntdll)

# msxml3_v1.tlb needs stdole2.tlb
add_dependencies(msxml3 stdole2)

add_cd_file(TARGET msxml3 DESTINATION reactos/system32 FOR all)
