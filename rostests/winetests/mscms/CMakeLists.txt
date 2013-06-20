
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(mscms_winetest profile.c testlist.c)
target_link_libraries(mscms_winetest wine)
set_module_type(mscms_winetest win32cui)
add_importlibs(mscms_winetest advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET mscms_winetest DESTINATION reactos/bin FOR all)
