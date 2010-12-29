
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(urlmon.dll urlmon.spec)

rpcproxy(urlmon urlmon_urlmon.idl)

list(APPEND SOURCE
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
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/urlmon.def)

add_library(urlmon SHARED ${SOURCE})
    

set_module_type(urlmon win32dll)

target_link_libraries(urlmon
    uuid
    wine
    ${PSEH_LIB})

add_importlibs(urlmon
    rpcrt4
    ole32
    oleaut32
    shlwapi
    shell32
    wininet
    user32
    advapi32
    msvcrt
    kernel32
    ntdll)


add_definitions(
    -DENTRY_PREFIX=URLMON_
    -DPROXY_DELEGATION
    -DREGISTER_PROXY_DLL
    -D_URLMON_)

# FIXME : this should be quotes enclosed 
add_definitions(-DPROXY_CLSID_IS={0x79EAC9F1,0xBAF9,0x11CE,{0x8C,0x82,0x00,0xAA,0x00,0x4B,0xA9,0x0B}})
#add_definitions(-DPROXY_CLSID_IS="{0x79EAC9F1,0xBAF9,0x11CE,{0x8C,0x82,0x00,0xAA,0x00,0x4B,0xA9,0x0B}}")

add_cab_target(urlmon 1)
add_importlib_target(urlmon.spec)
