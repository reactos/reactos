
remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS -D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/wine)

spec2def(combase.dll combase.spec ADD_IMPORTLIB)
add_idl_headers(combase_idlheader dcom.idl)
add_rpc_files(client irpcss.idl)

list(APPEND SOURCE
    apartment.c
    combase.c
    errorinfo.c
    hglobalstream.c
    malloc.c
    marshal.c
    roapi.c
    rpc.c
    string.c
    stubmanager.c
    usrmarshal.c
    ${CMAKE_CURRENT_BINARY_DIR}/combase_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/irpcss_c.c
)

add_definitions(-Dwcsdup=_wcsdup -Dwcsicmp=_wcsicmp -D_ROAPI_)

add_library(combase MODULE ${SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/combase.def)
set_module_type(combase win32dll)
if(MSVC)
    # Disable warning C4090: 'function': different 'const' qualifiers
    # Disable warning C4312: 'type cast': conversion from 'unsigned int' to 'void *' of greater size
    target_compile_options(combase PRIVATE /wd4090 /wd4312)
endif()
target_link_libraries(combase wine uuid pseh)
add_importlibs(combase advapi32 ole32 oleaut32 user32 gdi32 rpcrt4 msvcrt kernel32_vista kernel32 ntdll)
add_dependencies(combase combase_idlheader wineheaders rpcss)
add_cd_file(TARGET combase DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(combase) # CORE-5743: No ARRAY_SIZE macro
