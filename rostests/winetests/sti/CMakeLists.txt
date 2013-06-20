
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(sti_winetest sti.c testlist.c)
target_link_libraries(sti_winetest wine uuid)
set_module_type(sti_winetest win32cui)
add_importlibs(sti_winetest ole32 msvcrt kernel32 ntdll)
add_cd_file(TARGET sti_winetest DESTINATION reactos/bin FOR all)
