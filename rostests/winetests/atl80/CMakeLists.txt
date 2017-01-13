
add_definitions(-D_ATL_VER=_ATL_VER_80)
list(APPEND SOURCE atl.c testlist.c)
add_executable(atl80_winetest ${SOURCE} atl80.rc)
target_link_libraries(atl80_winetest uuid)
set_module_type(atl80_winetest win32cui)
add_importlibs(atl80_winetest atl80 oleaut32 ole32 advapi32 user32 msvcrt kernel32)
add_rostests_file(TARGET atl80_winetest)
