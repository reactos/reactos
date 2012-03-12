
add_definitions(
    -D__WINESRC__
    -D_WINE
    -Dclose=_close)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/lib/3rdparty/zlib)

spec2def(wininet.dll wininet.spec ADD_IMPORTLIB)

list(APPEND SOURCE
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
    ${CMAKE_CURRENT_BINARY_DIR}/wininet_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/wininet.def)

add_library(wininet SHARED
    ${SOURCE}
    rsrc.rc
    version.rc)

set_module_type(wininet win32dll)
target_link_libraries(wininet wine ${PSEH_LIB} zlib)
add_delay_importlibs(wininet secur32 crypt32)
add_importlibs(wininet mpr shlwapi shell32 user32 advapi32 ws2_32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wininet DESTINATION reactos/system32 FOR all)

if(NOT MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #allow_warnings(wininet)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-error")
endif()
