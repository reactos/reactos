
set_unicode()

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dinput ${CMAKE_CURRENT_SOURCE_DIR}/dinput.spec ${CMAKE_CURRENT_BINARY_DIR}/dinput.def)

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
    version.rc)

set_module_type(dinput win32dll)

target_link_libraries(dinput
    ${CMAKE_CURRENT_BINARY_DIR}/dinput.def
    dxguid
    uuid
    wine)

add_importlibs(dinput
    user32
    advapi32
    ole32
    winmm
    kernel32
    ntdll)

add_dependencies(dinput dinput_def)
add_livecd_target(dinput reactos/system32)
