
add_definitions(-D__WINESRC__ -DDIRECTINPUT_VERSION=0x0700)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(dinput.dll dinput.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    ansi.c
    config.c
    device.c
    dinput_main.c
    joystick_hid.c
    keyboard.c
    mouse.c
    dllmain.c)

add_library(dinput MODULE
    ${SOURCE}
    data_formats.c
    dinput.rc
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dinput.def)

add_library(dinput_data_formats data_formats.c)
add_dependencies(dinput_data_formats psdk)
set_module_type(dinput win32dll)
target_link_libraries(dinput dxguid uuid wine)
add_importlibs(dinput comctl32 ole32 setupapi hid user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET dinput DESTINATION reactos/system32 FOR all)
