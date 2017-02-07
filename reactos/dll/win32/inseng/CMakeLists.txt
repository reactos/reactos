
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()
spec2def(inseng.dll inseng.spec)

list(APPEND SOURCE
    inseng_main.c
    inseng.rc
    ${CMAKE_CURRENT_BINARY_DIR}/inseng_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/inseng.def)

add_library(inseng SHARED ${SOURCE})

set_module_type(inseng win32dll)

target_link_libraries(inseng uuid wine)

add_importlibs(inseng
    ole32
    advapi32
    msvcrt
    kernel32
    ntdll)

add_cd_file(TARGET inseng DESTINATION reactos/system32 FOR all)
