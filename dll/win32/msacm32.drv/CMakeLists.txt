
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(msacm32.drv msacm32.drv.spec)

add_library(msacm32.drv MODULE
    wavemap.c
    wavemap.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msacm32.def)

set_module_type(msacm32.drv win32dll UNICODE)
set_target_properties(msacm32.drv PROPERTIES SUFFIX "")
target_link_libraries(msacm32.drv wine)
add_importlibs(msacm32.drv msacm32 user32 winmm advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET msacm32.drv DESTINATION reactos/system32 FOR all)
