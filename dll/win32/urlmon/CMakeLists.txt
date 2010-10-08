
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(urlmon ${CMAKE_CURRENT_SOURCE_DIR}/urlmon.spec ${CMAKE_CURRENT_BINARY_DIR}/urlmon.def)

add_library(urlmon SHARED
    bindctx.c
    binding.c
    bindprot.c
    download.c
    file.c
    format.c
    ftp.c
    gopher.c
    http.c
    internet.c
    mimefilter.c
    mk.c
    protocol.c
    protproxy.c
    regsvr.c
    sec_mgr.c
    session.c
    umon.c
    umstream.c
    uri.c
    urlmon_main.c
    usrmarshal.c
    rsrc.rc)

set_module_type(urlmon win32dll)

target_link_libraries(urlmon
    ${CMAKE_CURRENT_BINARY_DIR}/urlmon.def
    urlmon_proxy
    uuid
    wine
    pseh)

add_importlibs(urlmon
    rpcrt4
    ole32
    oleaut32
    shlwapi
    shell32
    wininet
    user32
    advapi32
    kernel32
    ntdll)

add_dependencies(urlmon urlmon_def)
