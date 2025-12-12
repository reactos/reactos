
remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)

add_definitions(-DUSE_WINE_TODOS)

list(APPEND SOURCE
    domdoc.c
    saxreader.c
    schema.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c
    testlist.c)

add_executable(msxml6_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE})

add_dependencies(msxml6_winetest xmlparser_idlheader_test)
set_module_type(msxml6_winetest win32cui)
add_importlibs(msxml6_winetest ole32 oleaut32 msvcrt kernel32)

if(MSVC)
    add_importlibs(msxml6_winetest ntdll)
endif()

add_rostests_file(TARGET msxml6_winetest)
