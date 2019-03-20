
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(msgsm32.acm msgsm32.acm.spec)

list(APPEND SOURCE
    msgsm32.c
    ${CMAKE_CURRENT_BINARY_DIR}/msgsm32.def)

add_library(msgsm32.acm MODULE ${SOURCE})
set_module_type(msgsm32.acm win32dll)
set_target_properties(msgsm32.acm PROPERTIES SUFFIX "")
target_link_libraries(msgsm32.acm wine)
add_importlibs(msgsm32.acm winmm user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET msgsm32.acm DESTINATION reactos/system32 FOR all)
