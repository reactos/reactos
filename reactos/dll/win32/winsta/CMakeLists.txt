
spec2def(winsta.dll winsta.spec)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)

list(APPEND SOURCE
    logon.c
    main.c
    misc.c
    query.c
    security.c
    server.c
    ws.c
    winsta.h)

add_library(winsta SHARED
    ${SOURCE}
    winsta.rc
    ${CMAKE_CURRENT_BINARY_DIR}/winsta.def)

set_module_type(winsta win32dll)
target_link_libraries(winsta wine)
add_importlibs(winsta msvcrt kernel32 ntdll)
add_pch(winsta winsta.h SOURCE)
add_cd_file(TARGET winsta DESTINATION reactos/system32 FOR all)
