
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(printui ${CMAKE_CURRENT_SOURCE_DIR}/printui.spec ${CMAKE_CURRENT_BINARY_DIR}/printui.def)

add_library(printui SHARED printui.c printui.rc)

set_entrypoint(printui 0)

target_link_libraries(printui
    ${CMAKE_CURRENT_BINARY_DIR}/printui.def
    wine)

add_importlibs(printui shell32 kernel32 ntdll)
add_dependencies(printui printui_def psdk buildno_header)
