
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(hnetcfg SHARED
    apps.c
    hnetcfg.c
    manager.c
    policy.c
    port.c
    profile.c
    regsvr.c
    service.c
    hnetcfg.def)

if(NOT MSVC)
    set_source_files_properties(hnetcfg.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(hnetcfg win32dll)

target_link_libraries(hnetcfg wine)

add_importlibs(hnetcfg
    ole32
    advapi32
    kernel32
    ntdll)

add_cab_target(hnetcfg 1)
