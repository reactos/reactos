
add_definitions(-DBZ_NO_STDIO -DBZ_DECOMPRESS_ONLY)

spec2def(unbzip2.sys unbzip2.spec)

list(APPEND SOURCE
    bzlib.c
    randtable.c
    crctable.c
    decompress.c
    huffman.c
    dllmain.c
    bzlib_private.h)

add_library(bzip2 SHARED
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/unbzip2.def)

set_module_type(bzip2 kernelmodedriver)
add_importlibs(bzip2 ntoskrnl)
target_link_libraries(bzip2 libcntpr)
add_pch(bzip2 bzlib_private.h SOURCE)
set_target_properties(bzip2 PROPERTIES OUTPUT_NAME "unbzip2")
