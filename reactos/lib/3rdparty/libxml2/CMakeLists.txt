
include_directories(
    include
    ${REACTOS_SOURCE_DIR}/lib/3rdparty/libwin-iconv
    ${REACTOS_SOURCE_DIR}/include/reactos/libs/zlib)

add_definitions(
    -D__MINGW32__
    -DLIBXML_STATIC
    -D_REENTRANT
    -DHAVE_WIN32_THREADS
    -D_MBCS
    -D_WINDOWS
    -DWIN32
    -DHAVE_CONFIG_H
    -DICONV_CONST=const
    -D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    buf.c
    c14n.c
    catalog.c
    chvalid.c
    debugXML.c
    dict.c
    DOCBparser.c
    encoding.c
    entities.c
    error.c
    globals.c
    hash.c
    HTMLparser.c
    HTMLtree.c
    legacy.c
    list.c
    nanoftp.c
    nanohttp.c
    parser.c
    parserInternals.c
    pattern.c
    relaxng.c
    SAX.c
    SAX2.c
    threads.c
    tree.c
    uri.c
    valid.c
    xinclude.c
    xlink.c
    xmlIO.c
    xmlmemory.c
    xmlreader.c
    xmlregexp.c
    xmlmodule.c
    xmlsave.c
    xmlschemas.c
    xmlschemastypes.c
    xmlunicode.c
    xmlwriter.c
    xpath.c
    xpointer.c
    xmlstring.c
    libxml.h)

add_library(libxml2 ${SOURCE})
target_link_libraries(libxml2 oldnames)
add_pch(libxml2 libxml.h SOURCE)
add_dependencies(libxml2 psdk)

if(MSVC)
	add_target_compile_flags(libxml2 "/wd4189")
else()
    add_target_compile_flags(libxml2 "-w")
endif()
