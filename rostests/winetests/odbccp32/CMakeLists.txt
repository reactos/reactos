
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(odbccp32_winetest misc.c testlist.c)
target_link_libraries(odbccp32_winetest wine)
set_module_type(odbccp32_winetest win32cui)
add_importlibs(odbccp32_winetest odbccp32 msvcrt kernel32 ntdll)
add_cd_file(TARGET odbccp32_winetest DESTINATION reactos/bin FOR all)
