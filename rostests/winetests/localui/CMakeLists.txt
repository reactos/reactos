
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_executable(localui_winetest localui.c testlist.c)
target_link_libraries(localui_winetest wine)
set_module_type(localui_winetest win32cui)
add_importlibs(localui_winetest winspool msvcrt kernel32 ntdll)
add_cd_file(TARGET localui_winetest DESTINATION reactos/bin FOR all)
