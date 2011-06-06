
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    capture.c
    ds3d8.c
    ds3d.c
    dsound8.c
    dsound.c
    duplex.c
    propset.c
    testlist.c)

add_executable(dsound_winetest ${SOURCE})
target_link_libraries(dsound_winetest wine uuid dxguid)
set_module_type(dsound_winetest win32cui)
add_importlibs(dsound_winetest dsound ole32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET dsound_winetest DESTINATION reactos/bin FOR all)
