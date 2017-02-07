
add_typelib(winhttp_tlb.idl)

add_definitions(
    -D__WINESRC__
    -D_WINE)
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
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/winhttp.def)

set_source_files_properties(rsrc.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/winhttp_tlb.tlb)

add_library(winhttp SHARED ${SOURCE})

set_module_type(winhttp win32dll)
target_link_libraries(winhttp uuid wine)
add_delay_importlibs(winhttp oleaut32 ole32 crypt32)
add_importlibs(winhttp user32 advapi32 ws2_32 msvcrt kernel32 ntdll)

# wininet_tlb.tlb needs stdole2.tlb
add_dependencies(winhttp stdole2)

add_cd_file(TARGET winhttp DESTINATION reactos/system32 FOR all)
