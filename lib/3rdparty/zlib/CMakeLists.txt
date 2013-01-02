
add_definitions(-DNO_VIZ)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/zlib)

list(APPEND SOURCE
    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    infback.c
    inffast.c
    inflate.c
    inftrees.c
    trees.c
    uncompr.c
    zutil.c)

if(CMAKE_CROSSCOMPILING)
    add_library(zlib ${SOURCE})
else()
    add_library(zlibhost ${SOURCE})
endif()
