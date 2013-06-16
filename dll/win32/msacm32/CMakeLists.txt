
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(msacm32.dll msacm32.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    driver.c
    filter.c
    format.c
    internal.c
    msacm32_main.c
    pcmconverter.c
    stream.c
    ${CMAKE_CURRENT_BINARY_DIR}/msacm32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msacm32.def)

add_library(msacm32 SHARED ${SOURCE})
set_module_type(msacm32 win32dll ENTRYPOINT DllMain 12 UNICODE)
target_link_libraries(msacm32 wine)
add_importlibs(msacm32 msvcrt advapi32 user32 winmm kernel32 ntdll)
add_dependencies(msacm32 psdk)
add_cd_file(TARGET msacm32 DESTINATION reactos/system32 FOR all)

add_subdirectory(msacm32.drv)
