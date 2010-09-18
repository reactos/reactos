


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(printui ${CMAKE_CURRENT_SOURCE_DIR}/printui.spec ${CMAKE_CURRENT_BINARY_DIR}/printui.def)

add_library(printui SHARED printui.c printui.rc)

set_target_properties(printui PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(printui
    ${CMAKE_CURRENT_BINARY_DIR}/printui.def
    wine
    -lshell32
    mingw_common)

add_dependencies(printui printui_def psdk)
