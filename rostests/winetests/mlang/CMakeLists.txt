
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(mlang_winetest mlang.c testlist.c)
target_link_libraries(mlang_winetest wine uuid)
set_module_type(mlang_winetest win32cui)
add_importlibs(mlang_winetest mlang oleaut32 ole32 gdi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET mlang_winetest DESTINATION reactos/bin FOR all)
