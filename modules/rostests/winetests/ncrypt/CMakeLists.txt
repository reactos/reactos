
add_executable(ncrypt_winetest ncrypt.c)
set_module_type(ncrypt_winetest win32cui)
target_link_libraries(ncrypt_winetest winetestlib)
add_importlibs(ncrypt_winetest ncrypt bcrypt advapi32 user32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET ncrypt_winetest)
