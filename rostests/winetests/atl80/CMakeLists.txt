
add_definitions(-D_ATL_VER=_ATL_VER_80)
list(APPEND SOURCE atl.c testlist.c)
add_executable(atl80_winetest ${SOURCE})
target_link_libraries(atl80_winetest wine uuid)
set_module_type(atl80_winetest win32cui)
add_importlibs(atl80_winetest atl80 oleaut32 ole32 advapi32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET atl80_winetest DESTINATION reactos/bin FOR all)
