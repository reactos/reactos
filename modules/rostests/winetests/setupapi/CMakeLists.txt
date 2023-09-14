
add_definitions(
    -D__WINESRC__
    -Dstrcasecmp=_stricmp
    -Dstrncasecmp=_strnicmp
)

remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)

spec2def(coinst.dll coinst.spec)
add_library(coinst MODULE coinst.c ${CMAKE_CURRENT_BINARY_DIR}/coinst.def)
set_module_type(coinst win32dll)
add_importlibs(coinst msvcrt kernel32)

list(APPEND SOURCE
    devinst.c
    diskspace.c
    install.c
    misc.c
    parser.c
    query.c
    setupcab.c
    stringtable.c
    testlist.c)

set_property(SOURCE setupapi.rc PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/coinst.dll)

add_executable(setupapi_winetest ${SOURCE} setupapi.rc)
set_module_type(setupapi_winetest win32cui)
target_link_libraries(setupapi_winetest uuid)
add_importlibs(setupapi_winetest advapi32 cabinet setupapi user32 shell32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET setupapi_winetest)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(setupapi_winetest PRIVATE -Wno-format-overflow)
endif()
