
add_definitions(-D_ATL_VER=_ATL_VER_100)
list(APPEND SOURCE atl.c testlist.c)
add_executable(atl100_winetest ${SOURCE})
target_link_libraries(atl100_winetest wine uuid)
set_module_type(atl100_winetest win32cui)
add_importlibs(atl100_winetest atl100 oleaut32 ole32 advapi32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET atl100_winetest DESTINATION reactos/bin FOR all)
