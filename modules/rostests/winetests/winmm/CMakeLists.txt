
list(APPEND SOURCE
    capture.c
    generated.c
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

if(USE_CLANG_CL OR (NOT MSVC))
    target_compile_options(winmm_winetest PRIVATE "-Wno-format")
endif()

target_link_libraries(winmm_winetest dxguid)
set_module_type(winmm_winetest win32cui)
add_importlibs(winmm_winetest winmm user32 advapi32 ole32 msvcrt kernel32)
add_rostests_file(TARGET winmm_winetest)
