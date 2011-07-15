
add_definitions(-D_DLL -D__USE_CRTIMP)
add_executable(avifil32_winetest api.c testlist.c)
target_link_libraries(avifil32_winetest wine)
set_module_type(avifil32_winetest win32cui)
add_importlibs(avifil32_winetest avifil32 msvcrt kernel32)
add_cd_file(TARGET avifil32_winetest DESTINATION reactos/bin FOR all)
