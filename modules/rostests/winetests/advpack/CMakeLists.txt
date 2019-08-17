
add_definitions(-DUSE_WINE_TODOS)

if(GCC AND GCC_VERSION VERSION_GREATER 7)
    add_compile_flags("-Wno-format-overflow")
endif()

list(APPEND SOURCE
    advpack.c
    files.c
    install.c
    testlist.c)

add_executable(advpack_winetest ${SOURCE})
target_link_libraries(advpack_winetest)
set_module_type(advpack_winetest win32cui)
add_importlibs(advpack_winetest advpack cabinet advapi32 msvcrt kernel32)
add_rostests_file(TARGET advpack_winetest)
