
add_executable(devenum_winetest devenum.c testlist.c)
set_module_type(devenum_winetest win32cui)
add_importlibs(devenum_winetest advapi32 dsound msvfw32 oleaut32 ole32 winmm msdmo msvcrt kernel32)
add_rostests_file(TARGET devenum_winetest)
