
add_definitions(-DUSE_WINE_TODOS)
add_executable(wscript_winetest run.c testlist.c rsrc.rc)
target_link_libraries(wscript_winetest uuid)
set_module_type(wscript_winetest win32cui)
add_importlibs(wscript_winetest ole32 oleaut32 psapi advapi32 msvcrt kernel32)
add_cd_file(TARGET wscript_winetest DESTINATION reactos/bin FOR all)
