
include_directories(include)

add_definitions(
    -D__MINGW32__
    -DLIBXML_STATIC
    -D_REENTRANT
    -DHAVE_WIN32_THREADS
    -D_MBCS
    -D_WINDOWS
    -DWIN32
    -DHAVE_CONFIG_H
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
    xmlstring.c)

add_library(libxml2 ${SOURCE})
target_link_libraries(libxml2 oldnames)
add_dependencies(libxml2 psdk)

if(NOT MSVC)
    add_target_compile_flags(libxml2 "-Wno-format -Wno-pointer-sign")
    allow_warnings(libxml2)
endif()
