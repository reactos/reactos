
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(msvfw32_winetest msvfw.c testlist.c)
target_link_libraries(msvfw32_winetest wine)
set_module_type(msvfw32_winetest win32cui)
add_importlibs(msvfw32_winetest msvfw32 msvcrt kernel32 ntdll)
add_cd_file(TARGET msvfw32_winetest DESTINATION reactos/bin FOR all)
