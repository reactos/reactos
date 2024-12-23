
remove_definitions(
    -DWINVER=0x502
    -D_WIN32_WINNT=0x502)
add_definitions(
    -DWINVER=0x600
    -D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -D_WINE
    -D__ROS_LONG64__)

spec2def(wininet.dll wininet.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    cookie.c
    dialogs.c
    ftp.c
    gopher.c
    http.c
    internet.c
    utility.c)

list(APPEND PCH_SKIP_SOURCE
    # Sometimes wine uses nameless structs & unions. Sometimes not... */
    netconnection.c
    urlcache.c
    ${CMAKE_CURRENT_BINARY_DIR}/wininet_stubs.c)

add_library(wininet_inflate OBJECT inflate.c)
target_include_directories(wininet_inflate BEFORE PRIVATE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_dependencies(wininet_inflate psdk)

add_library(wininet MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    $<TARGET_OBJECTS:wininet_inflate>
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wininet.def)

if(MSVC)
    # Disable warning C4090: 'function': different 'const' qualifiers
    target_compile_options(wininet PRIVATE /wd4090)
endif()

set_module_type(wininet win32dll)
target_link_libraries(wininet wine ${PSEH_LIB} oldnames)

add_delay_importlibs(wininet secur32 crypt32 cryptui iphlpapi dhcpcsvc)
add_importlibs(wininet mpr shlwapi shell32 user32 advapi32 ws2_32 normaliz kernel32_vista msvcrt kernel32 ntdll)
add_pch(wininet precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET wininet DESTINATION reactos/system32 FOR all)
