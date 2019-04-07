
add_definitions(
    -D__WINESRC__
    -D_URLMON_
    -DENTRY_PREFIX=URLMON_
    -DPROXY_DELEGATION
    -DWINE_REGISTER_DLL)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(urlmon.dll urlmon.spec ADD_IMPORTLIB)
add_rpcproxy_files(urlmon_urlmon.idl)

set(_source_file ${CMAKE_CURRENT_SOURCE_DIR}/urlmon.inf)
set(_converted_file ${CMAKE_CURRENT_BINARY_DIR}/urlmon_utf16.inf)
add_custom_command(
    OUTPUT "${_converted_file}"
    COMMAND native-utf16le "${_source_file}" "${_converted_file}"
    DEPENDS native-utf16le "${_source_file}")

set_source_files_properties(${_converted_file} PROPERTIES GENERATED TRUE)
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
    usrmarshal.c
    precomp.h)

add_library(urlmon MODULE
    ${SOURCE}
    urlmon.rc
    ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
    ${CMAKE_CURRENT_BINARY_DIR}/urlmon_urlmon_p.c
    ${CMAKE_CURRENT_BINARY_DIR}/urlmon_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/urlmon.def)

set_module_type(urlmon win32dll)
target_link_libraries(urlmon uuid wine ${PSEH_LIB})
add_importlibs(urlmon rpcrt4 propsys ole32 oleaut32 shlwapi shell32 wininet user32 advapi32 advpack kernel32_vista msvcrt kernel32 ntdll)
add_pch(urlmon precomp.h SOURCE)
add_cd_file(TARGET urlmon DESTINATION reactos/system32 FOR all)
