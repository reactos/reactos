
list(APPEND SOURCE
    logon.c
    main.c
    misc.c
    query.c
    security.c
    server.c
    ws.c
    winsta.rc
    winsta.def)

if(NOT MSVC)
    set_source_files_properties(winsta.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

add_library(winsta SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/winsta_winsta.h.gch
    ${SOURCE})
    
set_module_type(winsta win32dll)

target_link_libraries(winsta wine)

add_importlibs(winsta msvcrt kernel32 ntdll)
add_pch(winsta ${CMAKE_CURRENT_SOURCE_DIR}/winsta.h ${SOURCE})

add_cab_target(winsta 1)
