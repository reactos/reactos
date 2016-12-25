
add_executable(oleacc_winetest main.c testlist.c)
set_module_type(oleacc_winetest win32cui)
target_link_libraries(oleacc_winetest uuid)
add_importlibs(oleacc_winetest oleacc oleaut32 ole32 user32 msvcrt kernel32)
add_cd_file(TARGET oleacc_winetest DESTINATION reactos/bin FOR all)
