
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib
                    inc)

list(APPEND SOURCE
    ${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/zlib/adler32.c
    ${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/zlib/crc32.c
    ${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/zlib/deflate.c
    ${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/zlib/inffast.c
    ${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/zlib/inflate.c
    ${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/zlib/inftrees.c
    ${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/zlib/trees.c
    ${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/zlib/zutil.c)

add_library(zlib_solo ${SOURCE})

add_definitions(-DZ_SOLO)
