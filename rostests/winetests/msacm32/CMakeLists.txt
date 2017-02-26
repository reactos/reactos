
add_executable(msacm32_winetest msacm.c testlist.c)
set_module_type(msacm32_winetest win32cui)
add_importlibs(msacm32_winetest msacm32 winmm msvcrt kernel32)
add_rostests_file(TARGET msacm32_winetest)
