
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    editor.c
    richole.c
    testlist.c
    txtsrv.c)

add_executable(riched20_winetest ${SOURCE})
target_link_libraries(riched20_winetest wine uuid)
set_module_type(riched20_winetest win32cui)
add_importlibs(riched20_winetest ole32 oleaut32 user32 gdi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET riched20_winetest DESTINATION reactos/bin FOR all)
