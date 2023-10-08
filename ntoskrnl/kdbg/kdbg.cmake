
#add_definitions(-DKDBG)

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE kdbg/i386/kdb_help.S)
    list(APPEND SOURCE kdbg/i386/i386-dis.c)
elseif(ARCH STREQUAL "amd64")
    list(APPEND ASM_SOURCE kdbg/amd64/kdb_help.S)
    list(APPEND SOURCE kdbg/i386/i386-dis.c)
elseif(ARCH STREQUAL "arm")
endif()

list(APPEND SOURCE
    kdbg/kdbg.c
    kdbg/kdb.c
    kdbg/kdb_cli.c
    kdbg/kdb_cmdhist.c
    kdbg/kdb_expr.c
    kdbg/kdb_print.c
    kdbg/kdb_symbols.c)
