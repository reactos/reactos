
add_executable(msctf_winetest inputprocessor.c testlist.c)
set_module_type(msctf_winetest win32cui)
add_importlibs(msctf_winetest ole32 user32 msvcrt kernel32)
add_rostests_file(TARGET msctf_winetest)
