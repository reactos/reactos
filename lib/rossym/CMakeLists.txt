
if(MSVC)
add_library(rossym dummy.c)
else()
add_definitions(-D_NTSYSTEM_)
list(APPEND SOURCE
    delete.c
    dwarf386.c
    dwarfabbrev.c
    dwarfaranges.c
    dwarfcfa.c
    dwarfinfo.c
    dwarfget.c
    dwarfopen.c
    dwarfpc.c
    dwarfpubnames.c
    find.c
    fromfile.c
    iofile.c
    init.c
    initkm.c
    initum.c
    pe.c
    zwfile.c)
add_library(rossym ${SOURCE})
add_dependencies(rossym psdk bugcodes)
endif()
