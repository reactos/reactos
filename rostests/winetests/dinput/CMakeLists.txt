
add_definitions(-DUSE_WINE_TODOS)

add_executable(dinput_winetest
    device.c
    dinput.c
    joystick.c
    keyboard.c
    mouse.c
    testlist.c)

target_link_libraries(dinput_winetest dinput_data_formats)
set_module_type(dinput_winetest win32cui)
add_importlibs(dinput_winetest dinput ole32 user32 msvcrt kernel32)
add_cd_file(TARGET dinput_winetest DESTINATION reactos/bin FOR all)
