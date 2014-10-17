
add_executable(icmp_winetest icmp.c testlist.c)
target_link_libraries(icmp_winetest wine uuid)
set_module_type(icmp_winetest win32cui)
add_importlibs(icmp_winetest icmp msvcrt kernel32 ntdll)
add_cd_file(TARGET icmp_winetest DESTINATION reactos/bin FOR all)
