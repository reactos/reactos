
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_executable(localspl_winetest localmon.c testlist.c)
set_module_type(localspl_winetest win32cui)
add_importlibs(localspl_winetest advapi32 msvcrt kernel32)
add_rostests_file(TARGET localspl_winetest)
