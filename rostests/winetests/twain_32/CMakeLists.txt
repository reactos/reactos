
add_executable(twain_32_winetest dsm.c testlist.c)
set_module_type(twain_32_winetest win32cui)
add_importlibs(twain_32_winetest user32 gdi32 msvcrt kernel32)
add_cd_file(TARGET twain_32_winetest DESTINATION reactos/bin FOR all)
