
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/win32ss/include
    ../include
    .)

list(APPEND SOURCE
    emfdc.c
    emfdrv.c
    enhmetafile.c
    metadc.c
    metafile.c
    rosglue.c)

add_library(winegdi ${SOURCE})
target_link_libraries(winegdi PRIVATE pseh)
add_dependencies(winegdi psdk)
