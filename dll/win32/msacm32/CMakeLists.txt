
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(msacm32.dll msacm32.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    driver.c
    filter.c
    format.c
    internal.c
    msacm32_main.c
    pcmconverter.c
    stream.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/msacm32_stubs.c)

add_library(msacm32 MODULE
    ${SOURCE}
    msacm.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msacm32.def)

set_module_type(msacm32 win32dll ENTRYPOINT DllMain 12 UNICODE)
target_link_libraries(msacm32 wine oldnames)
add_importlibs(msacm32 advapi32 user32 winmm msvcrt kernel32 ntdll)
add_pch(msacm32 precomp.h SOURCE)
add_cd_file(TARGET msacm32 DESTINATION reactos/system32 FOR all)
