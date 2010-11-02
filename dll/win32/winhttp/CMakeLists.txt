
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(winhttp SHARED
    cookie.c
    handle.c
    main.c
    net.c
    request.c
    session.c
    url.c
    winhttp.def)

if(NOT MSVC)
    set_source_files_properties(winhttp.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(winhttp win32dll)

target_link_libraries(winhttp wine)

add_importlibs(winhttp shlwapi wininet ws2_32 msvcrt crypt32 advapi32 kernel32 ntdll)

add_cab_target(winhttp 1)
