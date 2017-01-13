
add_definitions(-DUSE_WINE_TODOS)

list(APPEND SOURCE
    filedlg.c
    finddlg.c
    fontdlg.c
    itemdlg.c
    printdlg.c
    testlist.c)

add_executable(comdlg32_winetest ${SOURCE} rsrc.rc)
target_link_libraries(comdlg32_winetest uuid)
set_module_type(comdlg32_winetest win32cui)
add_importlibs(comdlg32_winetest comdlg32 winspool user32 gdi32 ole32 shell32 msvcrt kernel32)
add_rostests_file(TARGET comdlg32_winetest)
