
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    image.c
    integrity.c
    testlist.c)

add_executable(imagehlp_winetest ${SOURCE})
target_link_libraries(imagehlp_winetest wine uuid)
set_module_type(imagehlp_winetest win32cui)
add_importlibs(imagehlp_winetest advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET imagehlp_winetest DESTINATION reactos/bin FOR all)
