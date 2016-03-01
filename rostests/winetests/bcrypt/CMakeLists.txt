
add_executable(bcrypt_winetest bcrypt.c testlist.c)
set_module_type(bcrypt_winetest win32cui)
add_importlibs(bcrypt_winetest bcrypt user32 msvcrt kernel32)
add_cd_file(TARGET bcrypt_winetest DESTINATION reactos/bin FOR all)
