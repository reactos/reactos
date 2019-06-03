
add_definitions(-D_WINE)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

if(MSVC)
    # error C4312: 'type cast': conversion from 'DWORD' to 'HTASK' of greater size
    replace_compile_flags("/we4312" " ")
endif()

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
    winemm.h)

add_library(winmm MODULE
    ${SOURCE}
    winmm_res.rc)
spec2def(winmm winmm.dll winmm.spec ADD_IMPORTLIB)

set_module_type(winmm win32dll)
target_link_libraries(winmm wine ${PSEH_LIB})
add_importlibs(winmm advapi32 user32 msvcrt kernel32 ntdll)
add_pch(winmm winemm.h SOURCE)
add_cd_file(TARGET winmm DESTINATION reactos/system32 FOR all)

if(NOT MSVC)
    add_target_compile_flags(winmm "-Wno-unused-but-set-variable")
endif()

add_subdirectory(midimap)
