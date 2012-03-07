

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dinput.dll dinput.spec ADD_IMPORTLIB)

add_library(dinput SHARED
    data_formats.c
    device.c
    dinput_main.c
    effect_linuxinput.c
    joystick_linux.c
    joystick_linuxinput.c
    keyboard.c
    mouse.c
    regsvr.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dinput.def)

set_module_type(dinput win32dll UNICODE)

target_link_libraries(dinput
    dxguid
    uuid
    wine)

add_importlibs(dinput
    user32
    advapi32
    ole32
    winmm
    msvcrt
    kernel32
    ntdll)

add_cd_file(TARGET dinput DESTINATION reactos/system32 FOR all)

