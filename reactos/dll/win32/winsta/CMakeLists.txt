
spec2def(winsta.dll winsta.spec)

list(APPEND SOURCE
    logon.c
    main.c
    misc.c
    query.c
    security.c
    server.c
    ws.c
    winsta.rc
    ${CMAKE_CURRENT_BINARY_DIR}/winsta.def)

add_library(winsta SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/winsta_winsta.h.gch
    ${SOURCE})
    
set_module_type(winsta win32dll)
target_link_libraries(winsta wine)
add_importlibs(winsta msvcrt kernel32 ntdll)
add_pch(winsta ${CMAKE_CURRENT_SOURCE_DIR}/winsta.h ${SOURCE})
add_cd_file(TARGET winsta DESTINATION reactos/system32 FOR all)
