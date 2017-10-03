
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_executable(imm32_winetest imm32.c testlist.c)
set_module_type(imm32_winetest win32cui)
add_importlibs(imm32_winetest imm32 user32 msvcrt kernel32)
add_rostests_file(TARGET imm32_winetest)
