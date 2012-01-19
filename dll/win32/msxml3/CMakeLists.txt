
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x601)

add_definitions(
    -D__WINESRC__
    -D_WINE
    -DLIBXML_STATIC)

if(MSVC)
    add_compile_flags("/FIwine/typeof.h /FImsvc.h")
endif(MSVC)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()

spec2def(msxml3.dll msxml3.spec)

add_typelib(msxml3_v1.idl)

list(APPEND SOURCE
    attribute.c
    comment.c
    domdoc.c
    element.c
    factory.c
    main.c
    httprequest.c
    node.c
    nodelist.c
    nodemap.c
    parseerror.c
    pi.c
    queryresult.c
    regsvr.c
    schema.c
    text.c
    uuid.c
    xmldoc.c
    xmlelem.c
    saxreader.c
    bsc.c
    cdata.c
    dispex.c
    docfrag.c
    domimpl.c
    entityref.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msxml3_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msxml3.def)

if(MSVC)
    list(APPEND SOURCE msvc_uuid.c)
endif()

add_library(msxml3 SHARED ${SOURCE})

set_module_type(msxml3 win32dll)

set_source_files_properties(version.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/msxml3_v1.tlb)

target_link_libraries(msxml3
    libxml2
    uuid
    wine
    wineldr)

if(MSVC)
    target_link_libraries(msxml3 xml_uuids)
endif()

add_importlibs(msxml3
    msvcrt
    urlmon
    wininet
    ws2_32
    comctl32
    shell32
    shlwapi
    cabinet
    oleaut32
    ole32
    version
    user32
    gdi32
    advapi32
    kernel32
    ntdll)

# msxml3_v1.tlb needs stdole2.tlb
add_dependencies(msxml3 stdole2)

add_cd_file(TARGET msxml3 DESTINATION reactos/system32 FOR all)
