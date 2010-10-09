
add_definitions(-D__WINESRC__)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(localspl ${CMAKE_CURRENT_SOURCE_DIR}/localspl.spec ${CMAKE_CURRENT_BINARY_DIR}/localspl.def)

add_library(localspl SHARED
    localmon.c
    localspl_main.c
    provider.c
    localspl.rc)

set_module_type(localspl win32dll)

target_link_libraries(localspl
    ${CMAKE_CURRENT_BINARY_DIR}/localspl.def
    wine)

add_importlibs(localspl
    spoolss
    user32
    advapi32
    kernel32
    ntdll)

add_dependencies(localspl localspl_def)
