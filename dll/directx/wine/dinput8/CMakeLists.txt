
add_definitions(-D__WINESRC__ -DDIRECTINPUT_VERSION=0x0800)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(dinput8.dll dinput8.spec ADD_IMPORTLIB)
set(DINPUT_SOURCE_FOLDER ../dinput)

list(APPEND SOURCE
    ${DINPUT_SOURCE_FOLDER}/config.c
    ${DINPUT_SOURCE_FOLDER}/data_formats.c
    ${DINPUT_SOURCE_FOLDER}/device.c
    ${DINPUT_SOURCE_FOLDER}/dinput_main.c
    ${DINPUT_SOURCE_FOLDER}/effect_linuxinput.c
    ${DINPUT_SOURCE_FOLDER}/joystick.c
    ${DINPUT_SOURCE_FOLDER}/joystick_linux.c
    ${DINPUT_SOURCE_FOLDER}/joystick_linuxinput.c
    ${DINPUT_SOURCE_FOLDER}/joystick_osx.c
    ${DINPUT_SOURCE_FOLDER}/keyboard.c
    ${DINPUT_SOURCE_FOLDER}/mouse.c
    ${CMAKE_CURRENT_BINARY_DIR}/dinput8.def)

add_library(dinput8 MODULE
    ${SOURCE}
    ${DINPUT_SOURCE_FOLDER}/dinput.rc
    version.rc)

set_module_type(dinput8 win32dll)
target_link_libraries(dinput8 dxguid uuid wine)
add_importlibs(dinput8 comctl32 ole32 user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET dinput8 DESTINATION reactos/system32 FOR all)
