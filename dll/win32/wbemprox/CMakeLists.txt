
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(wbemprox.dll wbemprox.spec)

list(APPEND SOURCE
    builtin.c
    class.c
    main.c
    process.c
    qualifier.c
    query.c
    reg.c
    security.c
    service.c
    services.c
    sysrestore.c
    table.c
    wbemlocator.c)

BISON_TARGET(wql wql.y ${CMAKE_CURRENT_BINARY_DIR}/wql.tab.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c
    ${BISON_wql_OUTPUTS})

add_library(wbemprox MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/wbemprox.def)

set_module_type(wbemprox win32dll)
add_idl_reg_scripts(wbemprox registry wbemprox.idl)
target_link_libraries(wbemprox uuid wine wine_dll_register wine_dll_canunload oldnames)
add_importlibs(wbemprox iphlpapi oleaut32 ole32 advapi32 advapi32_vista user32 gdi32 version winspool ws2_32 rpcrt4 setupapi msvcrt kernel32 kernel32_vista ntdll)
add_dependencies(wbemprox d3d_idl_headers)
add_pch(wbemprox precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET wbemprox DESTINATION reactos/system32/wbem FOR all)
set_wine_module_FIXME(wbemprox)
