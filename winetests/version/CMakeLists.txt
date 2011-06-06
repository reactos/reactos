
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    info.c
    install.c
    version.rc
    testlist.c)

add_executable(version_winetest ${SOURCE})
target_link_libraries(version_winetest wine)
set_module_type(version_winetest win32cui)
add_importlibs(version_winetest version msvcrt kernel32 ntdll)
add_cd_file(TARGET version_winetest DESTINATION reactos/bin FOR all)
