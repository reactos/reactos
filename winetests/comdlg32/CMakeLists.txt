
add_definitions(-D__ROS_LONG64__)

list(APPEND SOURCE
    filedlg.c
    fontdlg.c
    printdlg.c
    testlist.c
    rsrc.rc)

add_executable(comdlg32_winetest ${SOURCE})
target_link_libraries(comdlg32_winetest wine)

if(MSVC)
    target_link_libraries(comdlg32_winetest uuid)
endif()

set_module_type(comdlg32_winetest win32cui)
add_importlibs(comdlg32_winetest comdlg32 winspool user32 gdi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET comdlg32_winetest DESTINATION reactos/bin FOR all)
