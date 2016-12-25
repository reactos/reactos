
add_executable(iphlpapi_winetest iphlpapi.c testlist.c)
target_link_libraries(iphlpapi_winetest wine)
set_module_type(iphlpapi_winetest win32cui)
add_importlibs(iphlpapi_winetest ws2_32 msvcrt kernel32 ntdll)
add_cd_file(TARGET iphlpapi_winetest DESTINATION reactos/bin FOR all)
