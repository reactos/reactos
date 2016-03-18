
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(twain_32.dll twain_32.spec)

list(APPEND SOURCE
    dsm_ctrl.c
    twain32_main.c
    twain_i.h)

add_library(twain_32 SHARED
    ${SOURCE}
    twain_32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/twain_32.def)

set_module_type(twain_32 win32dll)
target_link_libraries(twain_32 wine)
add_importlibs(twain_32 msvcrt kernel32 ntdll)
add_pch(twain_32 twain_i.h SOURCE)
add_cd_file(TARGET twain_32 DESTINATION reactos FOR all)
