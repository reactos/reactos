
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(cryptui_winetest cryptui.c testlist.c)
target_link_libraries(cryptui_winetest wine)
set_module_type(cryptui_winetest win32cui)
add_importlibs(cryptui_winetest cryptui crypt32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET cryptui_winetest DESTINATION reactos/bin FOR all)
