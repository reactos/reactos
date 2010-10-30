add_definitions(-DBZ_NO_STDIO -DBZ_DECOMPRESS_ONLY)

spec2def(bzip2 unbzip2.spec)

list(APPEND SOURCE
    bzlib.c
    randtable.c
    crctable.c
    decompress.c
    huffman.c
    dllmain.c
    ${CMAKE_CURRENT_BINARY_DIR}/unbzip2.def)

add_library(bzip2 SHARED ${SOURCE})

set_target_properties(bzip2 PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native" OUTPUT_NAME "unbzip2")

add_importlibs(bzip2 ntoskrnl)

add_dependencies(bzip2 bugcodes)