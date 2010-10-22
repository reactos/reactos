
set_unicode()

spec2def(powercfg ${CMAKE_CURRENT_SOURCE_DIR}/powercfg.spec ${CMAKE_CURRENT_BINARY_DIR}/powercfg.def)

add_library(powercfg SHARED
    powercfg.c
    powershemes.c
    alarms.c
    advanced.c
    hibernate.c
    powercfg.rc)


set_module_type(powercfg cpl)

target_link_libraries(powercfg ${CMAKE_CURRENT_BINARY_DIR}/powercfg.def)

add_importlibs(powercfg
    msvcrt
    user32
    powrprof
    comctl32
    shell32
    advapi32
    kernel32)

add_dependencies(powercfg powercfg_def)
add_livecd_target(powercfg reactos/system32)
