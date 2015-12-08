
add_definitions(-DUSE_WINE_TODOS)
add_executable(qedit_winetest mediadet.c testlist.c qedit.rc)
set_module_type(qedit_winetest win32cui)
add_importlibs(qedit_winetest oleaut32 ole32 msvcrt kernel32)
add_cd_file(TARGET qedit_winetest DESTINATION reactos/bin FOR all)
