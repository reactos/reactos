
add_definitions(-DUNICODE -D_UNICODE)

list(APPEND SOURCE
    preparsed.h
    hidparser.h
    hidpmem.h
    hidphelpers.h
    hidparser.c
    hidphelpers.c
    api.c
    hidp.c)

add_library(hidparser_um ${SOURCE})
add_importlibs(hidparser_um ntdll)
add_dependencies(hidparser_um bugcodes xdk)

add_library(hidparser_km ${SOURCE})
add_importlibs(hidparser_km ntoskrnl)
add_dependencies(hidparser_km bugcodes xdk)
