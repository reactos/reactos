
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(opengl32_winetest opengl.c testlist.c)
target_link_libraries(opengl32_winetest wine)
set_module_type(opengl32_winetest win32cui)
add_importlibs(opengl32_winetest opengl32 gdi32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET opengl32_winetest DESTINATION reactos/bin FOR all)
