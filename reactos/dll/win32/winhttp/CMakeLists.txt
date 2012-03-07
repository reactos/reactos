
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(winhttp.dll winhttp.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    cookie.c
    handle.c
    main.c
    net.c
    request.c
    session.c
    url.c
    ${CMAKE_CURRENT_BINARY_DIR}/winhttp.def)

add_library(winhttp SHARED ${SOURCE})

set_module_type(winhttp win32dll)
target_link_libraries(winhttp wine)
add_delay_importlibs(winhttp crypt32)
add_importlibs(winhttp shlwapi wininet ws2_32 msvcrt advapi32 kernel32 ntdll)

add_cd_file(TARGET winhttp DESTINATION reactos/system32 FOR all)

if(NOT MSVC)
    allow_warnings(winhttp)
endif()
