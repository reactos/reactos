
spec2def(winsta ${CMAKE_CURRENT_SOURCE_DIR}/winsta.spec ${CMAKE_CURRENT_BINARY_DIR}/winsta.def)

list(APPEND SOURCE
    logon.c
    main.c
    misc.c
    query.c
    security.c
    server.c
    ws.c
    winsta.rc)

add_library(winsta SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/winsta_winsta.h.gch
    ${SOURCE})
    
set_module_type(winsta win32dll)

target_link_libraries(winsta
    ${CMAKE_CURRENT_BINARY_DIR}/winsta.def
    wine)

add_importlibs(winsta kernel32 ntdll)
add_pch(winsta ${CMAKE_CURRENT_SOURCE_DIR}/winsta.h ${SOURCE})
add_dependencies(winsta winsta_def)
add_cab_target(winsta 1)
