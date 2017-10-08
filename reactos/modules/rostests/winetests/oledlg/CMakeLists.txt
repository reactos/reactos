
add_definitions(-DUSE_WINE_TODOS)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_executable(oledlg_winetest main.c testlist.c)
set_module_type(oledlg_winetest win32cui)
add_importlibs(oledlg_winetest oledlg ole32 user32 msvcrt kernel32)
add_rostests_file(TARGET oledlg_winetest)
