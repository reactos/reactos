
add_definitions(-DUSE_WINE_TODOS -DWIN32_LEAN_AND_MEAN=)

list(APPEND SOURCE
    ie.c
    intshcut.c
    webbrowser.c)

add_executable(ieframe_winetest ${SOURCE})
set_module_type(ieframe_winetest win32cui)
target_link_libraries(ieframe_winetest winetestlib oldnames)
add_importlibs(ieframe_winetest advapi32 ole32 oleaut32 gdi32 user32 msvcrt kernel32)
add_rostests_file(TARGET ieframe_winetest)
