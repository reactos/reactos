
add_definitions(-D_WINE)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

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
    ${CMAKE_CURRENT_BINARY_DIR}/winmm_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/winmm.def)

add_library(winmm SHARED
    ${SOURCE}
    winmm_res.rc)

if(NOT MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #allow_warnings(winmm)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-error")
endif()

set_module_type(winmm win32dll)
target_link_libraries(winmm wine ${PSEH_LIB})
add_importlibs(winmm advapi32 user32 msvcrt kernel32 ntdll)
add_subdirectory(midimap)
add_cd_file(TARGET winmm DESTINATION reactos/system32 FOR all)
