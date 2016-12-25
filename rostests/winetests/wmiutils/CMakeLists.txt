
add_definitions(
    -DUSE_WINE_TODOS
    -D__WINESRC__)

add_executable(wmiutils_winetest path.c testlist.c)
set_module_type(wmiutils_winetest win32cui)
add_importlibs(wmiutils_winetest ole32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wmiutils_winetest DESTINATION reactos/bin FOR all)
