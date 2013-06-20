
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_executable(imm32_winetest imm32.c testlist.c)
target_link_libraries(imm32_winetest wine)
set_module_type(imm32_winetest win32cui)
add_importlibs(imm32_winetest imm32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET imm32_winetest DESTINATION reactos/bin FOR all)
