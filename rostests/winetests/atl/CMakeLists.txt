
list(APPEND SOURCE
    atl_ax.c
    module.c
    registrar.c
    testlist.c)

add_executable(atl_winetest ${SOURCE})
target_link_libraries(atl_winetest wine uuid)
set_module_type(atl_winetest win32cui)
add_importlibs(atl_winetest ole32 user32 atl msvcrt kernel32 ntdll advapi32)
add_cd_file(TARGET atl_winetest DESTINATION reactos/bin FOR all)
