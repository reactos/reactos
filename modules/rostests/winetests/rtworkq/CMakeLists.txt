
add_executable(rtworkq_winetest rtworkq.c)
target_compile_definitions(rtworkq_winetest PRIVATE STANDALONE __WINESRC__)
set_module_type(rtworkq_winetest win32cui)
add_importlibs(rtworkq_winetest rtworkq ole32 oleaut32 msvcrt kernel32)
add_rostests_file(TARGET rtworkq_winetest)
