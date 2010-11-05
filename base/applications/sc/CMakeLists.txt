
set_unicode()
add_definitions(-DDEFINE_GUID)

list(APPEND SOURCE
    control.c
    create.c
    delete.c
    print.c
    query.c
    sc.c
    start.c
    usage.c
    sc.rc)

add_executable(sc ${CMAKE_CURRENT_BINARY_DIR}/sc_sc.h.gch ${SOURCE})

add_pch(sc ${CMAKE_CURRENT_SOURCE_DIR}/sc.h ${SOURCE})

set_module_type(sc win32cui)

add_importlibs(sc advapi32 msvcrt kernel32)

add_cab_target(sc 1)