
add_typelib(msxml3_v1 msxml3_v1.idl)
add_dependencies(msxml3_v1 stdole2)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x601)

add_definitions(
    -D__WINESRC__
    -D_WINE
    -DLIBXML_STATIC)

if(MSVC)
    add_compiler_flags(/FIwine/typeof.h)
endif(MSVC)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()

spec2def(msxml3.dll msxml3.spec)

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

add_library(msxml3 SHARED ${SOURCE})

set_module_type(msxml3 win32dll)

target_link_libraries(msxml3
    libxml2
    uuid
    wine
    wineldr)

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

add_dependencies(msxml3 msxml3_v1)
add_cd_file(TARGET msxml3 DESTINATION reactos/system32 FOR all)
