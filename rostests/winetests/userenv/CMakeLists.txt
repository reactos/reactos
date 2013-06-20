
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(userenv_winetest userenv.c testlist.c)
target_link_libraries(userenv_winetest wine)
set_module_type(userenv_winetest win32cui)
add_importlibs(userenv_winetest userenv advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET userenv_winetest DESTINATION reactos/bin FOR all)
