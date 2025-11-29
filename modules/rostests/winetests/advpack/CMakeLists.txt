
list(APPEND SOURCE
    advpack.c
    files.c
    install.c
    testlist.c)

add_executable(advpack_winetest ${SOURCE})

target_compile_definitions(advpack_winetest PRIVATE USE_WINE_TODOS)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(advpack_winetest PRIVATE -Wno-format-overflow)
endif()

set_module_type(advpack_winetest win32cui)
add_importlibs(advpack_winetest advpack cabinet advapi32 msvcrt kernel32)
add_rostests_file(TARGET advpack_winetest)
