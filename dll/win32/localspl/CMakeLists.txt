
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()

spec2def(localspl.dll localspl.spec)

list(APPEND SOURCE
    localmon.c
    localspl_main.c
    provider.c
    localspl.rc
    ${CMAKE_CURRENT_BINARY_DIR}/localspl_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/localspl.def)

add_library(localspl SHARED ${SOURCE})

set_module_type(localspl win32dll)

target_link_libraries(localspl wine)

add_importlibs(localspl
    spoolss
    user32
    advapi32
    msvcrt
    kernel32
    ntdll)

add_cd_file(TARGET localspl DESTINATION reactos/system32 FOR all)
