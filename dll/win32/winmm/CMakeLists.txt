
add_definitions(-D_WINE)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(winmm.dll winmm.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    driver.c
    joystick.c
    lolvldrv.c
    mci.c
    mmio.c
    playsound.c
    registry.c
    time.c
    winmm.c
    winemm.h
    ${CMAKE_CURRENT_BINARY_DIR}/winmm_stubs.c)

add_library(winmm SHARED
    ${SOURCE}
    winmm_res.rc
    ${CMAKE_CURRENT_BINARY_DIR}/winmm.def)

set_module_type(winmm win32dll HOTPATCHABLE)
target_link_libraries(winmm wine ${PSEH_LIB})
add_importlibs(winmm advapi32 user32 msvcrt kernel32 ntdll)
add_pch(winmm winemm.h SOURCE)
add_cd_file(TARGET winmm DESTINATION reactos/system32 FOR all)

if((NOT MSVC) AND (NOT CMAKE_C_COMPILER_ID STREQUAL "Clang"))
    add_target_compile_flags(winmm "-Wno-unused-but-set-variable")
endif()

add_subdirectory(midimap)
