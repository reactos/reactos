
add_definitions(-D__ROS_LONG64__)

list(APPEND SOURCE
    filedlg.c
    finddlg.c
    fontdlg.c
    itemdlg.c
    printdlg.c
    testlist.c
    rsrc.rc)

add_executable(comdlg32_winetest ${SOURCE})
target_link_libraries(comdlg32_winetest wine uuid)

set_module_type(comdlg32_winetest win32cui)
add_importlibs(comdlg32_winetest comdlg32 winspool user32 gdi32 msvcrt kernel32 ntdll ole32 shell32)
add_cd_file(TARGET comdlg32_winetest DESTINATION reactos/bin FOR all)
