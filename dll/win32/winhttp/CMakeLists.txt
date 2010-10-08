
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(winhttp ${CMAKE_CURRENT_SOURCE_DIR}/winhttp.spec ${CMAKE_CURRENT_BINARY_DIR}/winhttp.def)

add_library(winhttp SHARED
    cookie.c
    handle.c
    main.c
    net.c
    request.c
    session.c
    url.c)

set_module_type(winhttp win32dll)

target_link_libraries(winhttp
    ${CMAKE_CURRENT_BINARY_DIR}/winhttp.def
    wine)

add_importlibs(winhttp shlwapi wininet ws2_32 msvcrt kernel32 ntdll)
add_dependencies(winhttp winhttp_def)
