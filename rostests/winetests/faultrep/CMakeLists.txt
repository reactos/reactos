
add_executable(faultrep_winetest faultrep.c testlist.c)
set_module_type(faultrep_winetest win32cui)
add_importlibs(faultrep_winetest faultrep advapi32 msvcrt kernel32)
add_rostests_file(TARGET faultrep_winetest)
