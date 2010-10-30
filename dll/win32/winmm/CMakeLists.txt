
add_definitions(-D_WINE)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(winmm ${CMAKE_CURRENT_SOURCE_DIR}/winmm.spec ${CMAKE_CURRENT_BINARY_DIR}/winmm.def)

set_rc_compiler()

add_library(winmm SHARED
    driver.c
    joystick.c
    lolvldrv.c
    mci.c
    mmio.c
    playsound.c
    registry.c
    time.c
    winmm.c
    winmm_res.rc)

set_module_type(winmm win32dll)

target_link_libraries(winmm
    ${CMAKE_CURRENT_BINARY_DIR}/winmm.def
    wine
    pseh)

add_importlibs(winmm advapi32 user32 msvcrt kernel32 ntdll)
add_dependencies(winmm winmm_def)

add_subdirectory(midimap)
add_cab_target(winmm 1)
