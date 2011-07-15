
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(msvcrtd_winetest debug.c testlist.c)
target_link_libraries(msvcrtd_winetest wine)
set_module_type(msvcrtd_winetest win32cui)
add_importlibs(msvcrtd_winetest msvcrt kernel32 ntdll)
add_cd_file(TARGET msvcrtd_winetest DESTINATION reactos/bin FOR all)
