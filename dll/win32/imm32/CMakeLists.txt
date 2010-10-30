
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(imm32 ${CMAKE_CURRENT_SOURCE_DIR}/imm32.spec ${CMAKE_CURRENT_BINARY_DIR}/imm32.def)

add_library(imm32 SHARED imm.c version.rc)

set_module_type(imm32 win32dll)

target_link_libraries(imm32
    ${CMAKE_CURRENT_BINARY_DIR}/imm32.def
    wine)

add_importlibs(imm32 advapi32 user32 kernel32 ntdll)
add_dependencies(imm32 imm32_def)
add_cab_target(imm32 1)
