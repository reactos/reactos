
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(printui.dll printui.spec)

add_library(printui SHARED printui.c printui.rc
    ${CMAKE_CURRENT_BINARY_DIR}/printui.def)

set_entrypoint(printui 0)

target_link_libraries(printui wine)

add_importlibs(printui shell32 kernel32 ntdll)
add_dependencies(printui psdk buildno_header)
add_cab_target(printui 1)