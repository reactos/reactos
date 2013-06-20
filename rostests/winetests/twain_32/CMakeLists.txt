
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(twain_32_winetest dsm.c testlist.c)
target_link_libraries(twain_32_winetest wine)
set_module_type(twain_32_winetest win32cui)
add_importlibs(twain_32_winetest user32 gdi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET twain_32_winetest DESTINATION reactos/bin FOR all)
