
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(inseng.dll inseng.spec)

list(APPEND SOURCE
    inseng_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/inseng_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/inseng.def)

add_library(inseng SHARED ${SOURCE} inseng.rc)
set_module_type(inseng win32dll)
target_link_libraries(inseng uuid wine)
add_importlibs(inseng msvcrt kernel32 ntdll)
add_cd_file(TARGET inseng DESTINATION reactos/system32 FOR all)
