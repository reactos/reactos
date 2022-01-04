
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/win32ss/include)

add_definitions(-D__WINESRC__)

spec2def(imm32.dll imm32.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    candidate.c
    compstr.c
    guideline.c
    ime.c
    imm.c
    keymsg.c
    regword.c
    utils.c
    win3.c
    ${CMAKE_CURRENT_BINARY_DIR}/imm32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/imm32.def)

add_library(imm32 MODULE ${SOURCE} version.rc)
set_module_type(imm32 win32dll)
target_link_libraries(imm32 wine win32ksys)
add_importlibs(imm32 advapi32 user32 gdi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET imm32 DESTINATION reactos/system32 FOR all)
