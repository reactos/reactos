
add_definitions(-D_WINE)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
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

add_library(winmm MODULE
    ${SOURCE}
    winmm_res.rc
    ${CMAKE_CURRENT_BINARY_DIR}/winmm.def)


if(MSVC)
    # Disable warning C4090: 'function': different 'const' qualifiers
    # Disable warning C4312: 'type cast': conversion from 'DWORD' to 'HTASK' of greater size
    target_compile_options(winmm PRIVATE /wd4090 /wd4312)
endif()

set_module_type(winmm win32dll)
target_link_libraries(winmm wine ${PSEH_LIB} oldnames)
add_importlibs(winmm advapi32 user32 msvcrt kernel32 ntdll)
add_pch(winmm winemm.h SOURCE)
add_cd_file(TARGET winmm DESTINATION reactos/system32 FOR all)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(winmm PRIVATE -Wno-unused-but-set-variable)
endif()

add_subdirectory(midimap)
