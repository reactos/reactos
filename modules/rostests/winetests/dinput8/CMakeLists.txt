
add_definitions(-DUSE_WINE_TODOS )
list(APPEND SOURCE device.c dinput.c precomp.h)
add_executable(dinput8_winetest ${SOURCE} testlist.c)
target_link_libraries(dinput8_winetest dinput_data_formats)
set_module_type(dinput8_winetest win32cui)
add_importlibs(dinput8_winetest dinput8 ole32 user32 advapi32 msvcrt kernel32)
add_pch(dinput8_winetest precomp.h SOURCE)
add_rostests_file(TARGET dinput8_winetest)
