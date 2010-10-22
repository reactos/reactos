
add_definitions(-D__WINESRC__)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(hnetcfg ${CMAKE_CURRENT_SOURCE_DIR}/hnetcfg.spec ${CMAKE_CURRENT_BINARY_DIR}/hnetcfg.def)

add_library(hnetcfg SHARED
    apps.c
    hnetcfg.c
    manager.c
    policy.c
    port.c
    profile.c
    regsvr.c
    service.c)

set_module_type(hnetcfg win32dll)

target_link_libraries(hnetcfg
    ${CMAKE_CURRENT_BINARY_DIR}/hnetcfg.def
    wine)

add_importlibs(hnetcfg
    ole32
    advapi32
    kernel32
    ntdll)

add_dependencies(hnetcfg hnetcfg_def)
add_livecd_target(hnetcfg reactos/system32)
