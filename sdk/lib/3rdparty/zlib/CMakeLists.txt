
add_definitions(-DNO_VIZ)

if(MSVC)
    # C4267: '=': conversion from 'size_t' to 'unsigned int', possible loss of data
    add_compile_flags("/wd4267")
endif()

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

list(APPEND MINIZIP_SOURCE
    contrib/minizip/crypt.h
    contrib/minizip/ioapi.c
    contrib/minizip/ioapi.h
    contrib/minizip/iowin32.c
    contrib/minizip/iowin32.h
    contrib/minizip/unzip.c
    contrib/minizip/unzip.h
    contrib/minizip/zip.c
    contrib/minizip/zip.h)


if(CMAKE_CROSSCOMPILING)
    add_library(zlib ${SOURCE} ${SOLO_SOURCE})
    target_include_directories(zlib PRIVATE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib)
    add_library(zlib_solo ${SOLO_SOURCE})
    add_target_compile_definitions(zlib_solo Z_SOLO)
    target_include_directories(zlib_solo PRIVATE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib)
    add_library(minizip ${MINIZIP_SOURCE})
    add_dependencies(minizip psdk)
    target_include_directories(minizip PRIVATE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib)
else()
    add_library(zlibhost ${SOLO_SOURCE})
    add_target_compile_definitions(zlibhost Z_SOLO)
    target_include_directories(zlibhost PUBLIC ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib)
endif()
