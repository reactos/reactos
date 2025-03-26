
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__ -D__ROS_LONG64__)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
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
    table.c
    wbemlocator.c)

BISON_TARGET(wql wql.y ${CMAKE_CURRENT_BINARY_DIR}/wql.tab.c COMPILE_FLAGS "-p wql_")

list(APPEND PCH_SKIP_SOURCE
    guid.c
    ${BISON_wql_OUTPUTS})

add_library(wbemprox MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    wbemprox.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wbemprox.def)

set_source_files_properties(wbemprox.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/wbemprox.rgs)
set_module_type(wbemprox win32dll)
target_link_libraries(wbemprox uuid wine oldnames)
add_importlibs(wbemprox iphlpapi oleaut32 advapi32 user32 gdi32 version winspool ws2_32 rpcrt4 setupapi msvcrt kernel32 ntdll)
add_dependencies(wbemprox d3d_idl_headers)
add_pch(wbemprox precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET wbemprox DESTINATION reactos/system32/wbem FOR all)
