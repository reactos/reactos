
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(hnetcfg.dll hnetcfg.spec)

list(APPEND SOURCE
    apps.c
    hnetcfg.c
    manager.c
    policy.c
    port.c
    profile.c
    service.c
    hnetcfg.rc
    ${CMAKE_CURRENT_BINARY_DIR}/hnetcfg.def)

add_library(hnetcfg SHARED ${SOURCE})
set_module_type(hnetcfg win32dll)
target_link_libraries(hnetcfg wine uuid)

add_importlibs(hnetcfg
    ole32
    advapi32
    msvcrt
    kernel32
    ntdll)

add_cd_file(TARGET hnetcfg DESTINATION reactos/system32 FOR all)
