
add_definitions(-DNO_VIZ)

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

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wno-deprecated-non-prototype)
endif()

if(CMAKE_CROSSCOMPILING)
    add_library(zlib ${SOURCE} ${SOLO_SOURCE})
    target_link_libraries(zlib oldnames)
    target_include_directories(zlib PRIVATE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib)
    add_library(zlib_solo ${SOLO_SOURCE})
    target_compile_definitions(zlib_solo PRIVATE Z_SOLO)
    target_include_directories(zlib_solo PRIVATE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib)
    add_library(minizip ${MINIZIP_SOURCE})
    add_dependencies(minizip psdk)
    target_include_directories(minizip PRIVATE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib)
else()
    add_library(zlibhost ${SOLO_SOURCE})
    target_compile_definitions(zlibhost PRIVATE Z_SOLO)
    target_include_directories(zlibhost PUBLIC ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib)
endif()
