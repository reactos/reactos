
remove_definitions(-D_WIN32_WINNT=0x502)
add_executable(bcrypt_winetest bcrypt.c testlist.c)
set_module_type(bcrypt_winetest win32cui)
add_importlibs(bcrypt_winetest bcrypt advapi32 user32 msvcrt kernel32)
add_rostests_file(TARGET bcrypt_winetest)
