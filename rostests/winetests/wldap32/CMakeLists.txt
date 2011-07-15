
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(wldap32_winetest parse.c testlist.c)
target_link_libraries(wldap32_winetest wine)
set_module_type(wldap32_winetest win32cui)
add_importlibs(wldap32_winetest wldap32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wldap32_winetest DESTINATION reactos/bin FOR all)
