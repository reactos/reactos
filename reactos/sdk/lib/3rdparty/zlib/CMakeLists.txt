
add_definitions(-DNO_VIZ -DZ_SOLO)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib)

list(APPEND SOURCE
    adler32.c
    compress.c
    crc32.c
    deflate.c
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
