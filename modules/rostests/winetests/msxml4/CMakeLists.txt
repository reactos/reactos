
remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)

add_definitions(-DUSE_WINE_TODOS)

list(APPEND SOURCE
    domdoc.c
    schema.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c
    testlist.c)

add_executable(msxml4_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE})

set_module_type(msxml4_winetest win32cui)
add_importlibs(msxml4_winetest ole32 oleaut32 msvcrt kernel32)

if(MSVC)
    add_importlibs(msxml4_winetest ntdll)
endif()

add_rostests_file(TARGET msxml4_winetest)
