
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)
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
    wbemlocator.c
    wql.tab.c
    wbemprox_private.h)

add_library(wbemprox SHARED
    ${SOURCE}
    guid.c
    wbemprox.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wbemprox.def)

set_source_files_properties(wbemprox.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/wbemprox.rgs)
set_module_type(wbemprox win32dll)
target_link_libraries(wbemprox uuid wine)
add_importlibs(wbemprox iphlpapi oleaut32 advapi32 user32 gdi32 version winspool ws2_32 msvcrt kernel32 ntdll)
add_dependencies(wbemprox d3d_idl_headers)
add_pch(wbemprox wbemprox_private.h SOURCE)
add_cd_file(TARGET wbemprox DESTINATION reactos/system32/wbem FOR all)
