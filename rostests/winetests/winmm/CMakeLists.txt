
list(APPEND SOURCE
    capture.c
    joystick.c
    mci.c
    mcicda.c
    midi.c
    mixer.c
    mmio.c
    timer.c
    testlist.c
    wave.c)

add_executable(winmm_winetest ${SOURCE})
target_link_libraries(winmm_winetest dxguid)
set_module_type(winmm_winetest win32cui)
add_importlibs(winmm_winetest winmm user32 advapi32 msvcrt kernel32)
add_cd_file(TARGET winmm_winetest DESTINATION reactos/bin FOR all)

if(NOT MSVC)
    add_target_compile_flags(winmm_winetest "-Wno-format")
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
        add_target_compile_flags(winmm_winetest "-Wno-unused-but-set-variable")
    endif()
endif()
