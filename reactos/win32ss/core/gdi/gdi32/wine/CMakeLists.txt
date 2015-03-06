
include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/win32ss/core/include
    ../include
    .)

list(APPEND SOURCE
    enhmetafile.c
    metafile.c
    enhmfdrv/bitblt.c
    enhmfdrv/dc.c
    enhmfdrv/graphics.c
    enhmfdrv/init.c
    enhmfdrv/objects.c
    mfdrv/bitblt.c
    mfdrv/dc.c
    mfdrv/graphics.c
    mfdrv/init.c
    mfdrv/objects.c
    mfdrv/text.c
    rosglue.c
)

add_library(winegdi ${SOURCE})
