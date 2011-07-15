
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    imalloc.c
    prop.c
    util.c
    testlist.c)

add_executable(mapi32_winetest ${SOURCE})
target_link_libraries(mapi32_winetest wine uuid)
set_module_type(mapi32_winetest win32cui)
add_importlibs(mapi32_winetest advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET mapi32_winetest DESTINATION reactos/bin FOR all)
