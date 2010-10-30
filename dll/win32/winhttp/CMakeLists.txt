
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(winhttp.dll winhttp.spec)

add_library(winhttp SHARED
    cookie.c
    handle.c
    main.c
    net.c
    request.c
    session.c
    url.c
    ${CMAKE_CURRENT_BINARY_DIR}/winhttp.def)

set_module_type(winhttp win32dll)

target_link_libraries(winhttp wine)

add_importlibs(winhttp shlwapi wininet ws2_32 msvcrt crypt32 advapi32 kernel32 ntdll)

add_cab_target(winhttp 1)
