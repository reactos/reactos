
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_executable(localui_winetest localui.c testlist.c)
set_module_type(localui_winetest win32cui)
add_importlibs(localui_winetest winspool msvcrt kernel32)
add_rostests_file(TARGET localui_winetest)
