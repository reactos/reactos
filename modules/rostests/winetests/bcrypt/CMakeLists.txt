
remove_definitions(-D_WIN32_WINNT=0x502)
add_executable(bcrypt_winetest bcrypt.c testlist.c)
set_module_type(bcrypt_winetest win32cui)
# bcrypt is available on WVista (and ReactOS), not on WS03.
# advapi32/RegGetValueW() is available on WXP-Pro-x64 and WS03sp1, not on WXP.
add_delay_importlibs(bcrypt_winetest bcrypt advapi32)
add_importlibs(bcrypt_winetest user32 msvcrt kernel32)
add_rostests_file(TARGET bcrypt_winetest)
