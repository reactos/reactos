
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(msacm32_winetest msacm.c testlist.c)
target_link_libraries(msacm32_winetest wine)
set_module_type(msacm32_winetest win32cui)
add_importlibs(msacm32_winetest msacm32 msvcrt kernel32 ntdll)
add_cd_file(TARGET msacm32_winetest DESTINATION reactos/bin FOR all)
