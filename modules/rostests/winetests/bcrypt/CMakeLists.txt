add_executable(bcrypt_winetest bcrypt.c testlist.c)
set_module_type(bcrypt_winetest win32cui TARGET_VERSION vista)
add_importlibs(bcrypt_winetest bcrypt advapi32 user32 msvcrt kernel32)
add_rostests_file(TARGET bcrypt_winetest)
