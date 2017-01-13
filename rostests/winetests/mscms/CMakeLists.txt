
add_definitions(-DUSE_WINE_TODOS)
add_executable(mscms_winetest profile.c testlist.c)
set_module_type(mscms_winetest win32cui)
add_importlibs(mscms_winetest msvcrt kernel32)
add_rostests_file(TARGET mscms_winetest)
