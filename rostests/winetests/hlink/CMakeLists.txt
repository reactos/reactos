
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    browse_ctx.c
    hlink.c
    testlist.c)

add_executable(hlink_winetest ${SOURCE})
target_link_libraries(hlink_winetest wine uuid)
set_module_type(hlink_winetest win32cui)
add_importlibs(hlink_winetest hlink ole32 msvcrt kernel32 ntdll)
add_cd_file(TARGET hlink_winetest DESTINATION reactos/bin FOR all)
