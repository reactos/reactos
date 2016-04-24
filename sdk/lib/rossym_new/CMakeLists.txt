
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
    init.c
    initum.c
    pe.c
    zwfile.c)
add_library(rossym ${SOURCE})
add_pch(rossym precomp.h)
add_dependencies(rossym psdk bugcodes)
