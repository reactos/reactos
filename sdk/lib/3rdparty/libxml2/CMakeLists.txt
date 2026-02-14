
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/libwin-iconv
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib)

add_definitions(
    -D__MINGW32__
    -DLIBXML_STATIC
    -D_REENTRANT
    -D_MBCS
    -D_WINDOWS
    -DWIN32
    -DHAVE_CONFIG_H
    -DICONV_CONST=const
    -D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    buf.c
    catalog.c
    chvalid.c
    debugXML.c
    dict.c
    encoding.c
    entities.c
    error.c
    globals.c
    hash.c
    HTMLparser.c
    HTMLtree.c
    list.c
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
    xmlIO.c
    xmlmemory.c
    xmlreader.c
    xmlregexp.c
    xmlsave.c
    xmlschemas.c
    xmlschemastypes.c
    xmlunicode.c
    xpath.c
    xpointer.c
    xmlstring.c
    libxml.h)

add_library(libxml2 ${SOURCE})

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    # Unreferenced local variable
    remove_target_compile_option(libxml2 "/we4101")
    target_compile_options(libxml2 PRIVATE "/wd4101")
    # Local variable initialized but not referenced
    remove_target_compile_option(libxml2 "/we4189")
    # warning C4996: 'function': was declared deprecated
    target_compile_options(libxml2 PRIVATE "/wd4996")
    # warning C4090: 'function': different 'const' qualifiers
    target_compile_options(libxml2 PRIVATE "/wd4090")
    if(ARCH STREQUAL "amd64" OR ARCH STREQUAL "arm64")
        # '=': conversion from 'size_t' to 'int', possible loss of data
        target_compile_options(libxml2 PRIVATE "/wd4267")
    endif()
else()
    target_compile_options(libxml2 PRIVATE "-w")
endif()

target_include_directories(libxml2
    PRIVATE include/private
    PUBLIC include;include/libxml
)

target_link_libraries(libxml2 oldnames)
add_pch(libxml2 libxml.h SOURCE)
add_dependencies(libxml2 psdk)
