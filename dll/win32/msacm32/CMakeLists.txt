
set_unicode()

add_definitions(-D_DLL -D__USE_CRTIMP)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(msacm32 ${CMAKE_CURRENT_SOURCE_DIR}/msacm32.spec ${CMAKE_CURRENT_BINARY_DIR}/msacm32.def)

add_library(msacm32 SHARED
    driver.c
    filter.c
    format.c
    internal.c
    msacm32_main.c
    pcmconverter.c
    stream.c)

set_module_type(msacm32 win32dll)

target_link_libraries(msacm32
    ${CMAKE_CURRENT_BINARY_DIR}/msacm32.def
    wine)

add_importlibs(msacm32 msvcrt advapi32 user32 winmm kernel32 ntdll)
add_dependencies(msacm32 msacm32_def)

add_subdirectory(msacm32.drv)
