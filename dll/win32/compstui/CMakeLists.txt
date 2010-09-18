


add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(compstui ${CMAKE_CURRENT_SOURCE_DIR}/compstui.spec ${CMAKE_CURRENT_BINARY_DIR}/compstui.def)

add_library(compstui SHARED compstui_main.c)

set_target_properties(compstui PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(compstui
    ${CMAKE_CURRENT_BINARY_DIR}/compstui.def
    wine
    mingw_common)

add_dependencies(compstui compstui_def psdk)
