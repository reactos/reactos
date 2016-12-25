
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_executable(winspool_winetest info.c testlist.c)
set_module_type(winspool_winetest win32cui)
add_importlibs(winspool_winetest winspool advapi32 msvcrt kernel32)
add_cd_file(TARGET winspool_winetest DESTINATION reactos/bin FOR all)
