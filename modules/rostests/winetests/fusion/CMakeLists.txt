
list(APPEND SOURCE
    asmcache.c
    asmenum.c
    asmname.c
    fusion.c
    testlist.c)

add_executable(fusion_winetest ${SOURCE})

target_compile_definitions(fusion_winetest PRIVATE USE_WINE_TODOS)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(fusion_winetest PRIVATE -Wno-format-overflow)
endif()

set_module_type(fusion_winetest win32cui)
add_importlibs(fusion_winetest user32 msvcrt kernel32)
add_rostests_file(TARGET fusion_winetest)
