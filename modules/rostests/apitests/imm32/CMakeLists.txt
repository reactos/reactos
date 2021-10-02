
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    clientimc.c
    himc.c
    imcc.c
    ImmGetImeInfoEx.c
    ImmIsUIMessage.c
    testlist.c)

add_executable(imm32_apitest ${SOURCE})
target_link_libraries(imm32_apitest wine ${PSEH_LIB})
set_module_type(imm32_apitest win32cui)
add_importlibs(imm32_apitest imm32 msvcrt user32 kernel32 ntdll)
add_rostests_file(TARGET imm32_apitest)
