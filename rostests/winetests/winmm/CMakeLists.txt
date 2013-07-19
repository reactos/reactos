
add_definitions(-D__ROS_LONG64__)

list(APPEND SOURCE
    capture.c
    mci.c
    mixer.c
    mmio.c
    timer.c
    testlist.c
    wave.c)

add_executable(winmm_winetest ${SOURCE})
target_link_libraries(winmm_winetest wine dxguid)
set_module_type(winmm_winetest win32cui)
add_importlibs(winmm_winetest winmm user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET winmm_winetest DESTINATION reactos/bin FOR all)

if(NOT MSVC)
    add_target_compile_flags(winmm_winetest "-Wno-format -Wno-unused-but-set-variable")
endif()
