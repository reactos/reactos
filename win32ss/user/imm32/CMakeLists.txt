
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/win32ss/include)

spec2def(imm32.dll imm32.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    candidate.c
    compstr.c
    ctf.c
    guideline.c
    ime.c
    imemenu.c
    imepro.c
    imm.c
    install.c
    keymsg.c
    regword.c
    softkbd.c
    utils.c
    win3.c
    ${CMAKE_CURRENT_BINARY_DIR}/imm32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/imm32.def)

file(GLOB imm32_rc_deps ${CMAKE_CURRENT_SOURCE_DIR}/res/*.*)
add_rc_deps(imm32.rc ${imm32_rc_deps})

add_library(imm32 MODULE ${SOURCE} imm32.rc)
set_module_type(imm32 win32dll UNICODE ENTRYPOINT ImmDllInitialize 12)
target_link_libraries(imm32 wine win32ksys uuid)
add_importlibs(imm32 advapi32 user32 gdi32 kernel32 ntdll)
add_cd_file(TARGET imm32 DESTINATION reactos/system32 FOR all)
