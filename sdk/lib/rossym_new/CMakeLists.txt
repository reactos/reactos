
add_definitions(-D_NTSYSTEM_)

list(APPEND SOURCE
    delete.c
    dwarfabbrev.c
    dwarfaranges.c
    dwarfcfa.c
    dwarfget.c
    dwarfinfo.c
    dwarfopen.c
    dwarfpc.c
    dwarfpubnames.c
    find.c
    fromfile.c
    frommem.c
    init.c
    initkm.c
    iofile.c
    pe.c
    zwfile.c
    precomp.h)

add_library(rossym ${SOURCE})
add_pch(rossym precomp.h SOURCE)
add_dependencies(rossym psdk bugcodes)
target_compile_definitions(rossym PUBLIC __ROS_DWARF__ __ROS_ROSSYM__)
