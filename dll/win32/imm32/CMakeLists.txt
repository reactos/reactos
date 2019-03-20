
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(imm32.dll imm32.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    imm.c
    ${CMAKE_CURRENT_BINARY_DIR}/imm32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/imm32.def)

add_library(imm32 MODULE ${SOURCE} version.rc)
set_module_type(imm32 win32dll)
target_link_libraries(imm32 wine)
add_importlibs(imm32 advapi32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET imm32 DESTINATION reactos/system32 FOR all)
