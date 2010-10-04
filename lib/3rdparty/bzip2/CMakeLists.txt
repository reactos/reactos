add_definitions(-DBZ_NO_STDIO -DBZ_DECOMPRESS_ONLY)

spec2def(bzip2 ${CMAKE_CURRENT_SOURCE_DIR}/unbzip2.spec ${CMAKE_CURRENT_BINARY_DIR}/unbzip2.def)

list(APPEND SOURCE
    bzlib.c
    randtable.c
    crctable.c
    decompress.c
    huffman.c
    dllmain.c)

add_library(bzip2 SHARED ${SOURCE})

set_target_properties(bzip2 PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native" OUTPUT_NAME "unbzip2")

target_link_libraries(bzip2 ${CMAKE_CURRENT_BINARY_DIR}/unbzip2.def)

add_importlibs(bzip2 ntoskrnl)

add_dependencies(bzip2 bzip2_def bugcodes)