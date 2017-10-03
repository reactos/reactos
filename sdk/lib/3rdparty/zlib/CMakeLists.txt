
add_definitions(-DNO_VIZ)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib)

list(APPEND SOLO_SOURCE
    adler32.c
    crc32.c
    deflate.c
    inffast.c
    inflate.c
    inftrees.c
    trees.c
    zutil.c)

list(APPEND SOURCE
    compress.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    infback.c
    uncompr.c)

if(CMAKE_CROSSCOMPILING)
    add_library(zlib ${SOURCE} ${SOLO_SOURCE})
    add_library(zlib_solo ${SOLO_SOURCE})
    add_target_compile_definitions(zlib_solo Z_SOLO)
else()
    add_library(zlibhost ${SOLO_SOURCE})
    add_target_compile_definitions(zlibhost Z_SOLO)
endif()
