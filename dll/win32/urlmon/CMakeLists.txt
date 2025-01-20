
add_definitions(
    -D__WINESRC__
    -D_URLMON_
    -DENTRY_PREFIX=URLMON_
    -DPROXY_DELEGATION
    -DWINE_REGISTER_DLL
    -DPROXY_CLSID_IS={0x79EAC9F1,0xBAF9,0x11CE,{0x8C,0x82,0x00,0xAA,0x00,0x4B,0xA9,0x0B}})

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(urlmon.dll urlmon.spec ADD_IMPORTLIB)
add_rpcproxy_files(urlmon_urlmon.idl)

set(_source_file ${CMAKE_CURRENT_SOURCE_DIR}/urlmon.inf)
set(_converted_file ${CMAKE_CURRENT_BINARY_DIR}/urlmon_utf16.inf)
utf16le_convert(${_source_file} ${_converted_file})
set_source_files_properties(urlmon.rc PROPERTIES OBJECT_DEPENDS "${_converted_file}")

list(APPEND SOURCE
    axinstall.c
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
    sec_mgr.c
    session.c
    umon.c
    umstream.c
    uri.c
    urlmon_main.c
    usrmarshal.c)

list(APPEND PCH_SKIP_SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
    ${CMAKE_CURRENT_BINARY_DIR}/urlmon_urlmon_p.c
    ${CMAKE_CURRENT_BINARY_DIR}/urlmon_stubs.c)

add_library(urlmon MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    urlmon.rc
    ${CMAKE_CURRENT_BINARY_DIR}/urlmon.def)

set_module_type(urlmon win32dll)
target_link_libraries(urlmon uuid wine ${PSEH_LIB} oldnames)
add_delay_importlibs(urlmon advpack)
add_importlibs(urlmon rpcrt4 propsys ole32 oleaut32 shlwapi shell32 wininet user32 advapi32 kernel32_vista msvcrt kernel32 ntdll)
add_pch(urlmon precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET urlmon DESTINATION reactos/system32 FOR all)
