
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(cryptnet_winetest cryptnet.c testlist.c)
target_link_libraries(cryptnet_winetest wine)
set_module_type(cryptnet_winetest win32cui)
add_importlibs(cryptnet_winetest cryptnet crypt32 msvcrt kernel32 ntdll)
add_cd_file(TARGET cryptnet_winetest DESTINATION reactos/bin FOR all)
