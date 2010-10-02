
add_definitions(-D__WINESRC__ -D_WINE)
add_definitions(-Dclose=_close)

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/lib/3rdparty/zlib)

spec2def(wininet ${CMAKE_CURRENT_SOURCE_DIR}/wininet.spec ${CMAKE_CURRENT_BINARY_DIR}/wininet.def)

set_rc_compiler()

add_library(wininet SHARED
    cookie.c
    dialogs.c
    ftp.c
    gopher.c
    http.c
    internet.c
    netconnection.c
    urlcache.c
    utility.c
    wininet_main.c
    rsrc.rc
    version.rc)

set_module_type(wininet win32dll)

target_link_libraries(wininet
    ${CMAKE_CURRENT_BINARY_DIR}/wininet.def
    wine
    zlib
    pseh)

add_importlibs(wininet mpr shlwapi shell32 user32 advapi32 secur32 crypt32 ws2_32 msvcrt kernel32 ntdll)
add_dependencies(wininet wininet_def psdk buildno_header)
