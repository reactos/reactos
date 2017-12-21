
add_definitions(-DUSE_WINE_TODOS)

list(APPEND SOURCE
    device.c
    dinput.c
    joystick.c
    keyboard.c
    mouse.c
    precomp.h)

add_executable(dinput_winetest ${SOURCE} testlist.c)
target_link_libraries(dinput_winetest dinput_data_formats)
set_module_type(dinput_winetest win32cui)
add_importlibs(dinput_winetest dinput ole32 user32 msvcrt kernel32)
add_pch(dinput_winetest precomp.h SOURCE)
add_rostests_file(TARGET dinput_winetest)
