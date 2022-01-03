add_executable(winspool_winetest info.c testlist.c)
set_module_type(winspool_winetest win32cui TARGET_VERSION vista)
add_importlibs(winspool_winetest winspool advapi32 msvcrt kernel32)
add_rostests_file(TARGET winspool_winetest)
