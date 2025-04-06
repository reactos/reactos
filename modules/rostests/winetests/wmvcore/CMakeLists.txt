
add_definitions(
    -DUSE_WINE_TODOS
)

add_executable(wmvcore_winetest wmvcore.c testlist.c)
set_module_type(wmvcore_winetest win32cui)
add_importlibs(wmvcore_winetest ole32 wmvcore msvcrt kernel32)
add_rostests_file(TARGET wmvcore_winetest)
set_wine_module(wmvcore_winetest)
