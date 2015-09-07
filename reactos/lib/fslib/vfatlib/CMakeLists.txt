
list(APPEND SOURCE
    check/boot.c
    check/check.c
    check/common.c
    check/fat.c
    check/file.c
    check/io.c
    check/lfn.c
    fat12.c
    fat16.c
    fat32.c
    vfatlib.c
    vfatlib.h)

add_library(vfatlib ${SOURCE})
add_dependencies(vfatlib xdk)
add_pch(vfatlib vfatlib.h SOURCE)
