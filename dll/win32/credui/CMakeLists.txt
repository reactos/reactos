


add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(credui ${CMAKE_CURRENT_SOURCE_DIR}/credui.spec ${CMAKE_CURRENT_BINARY_DIR}/credui.def)

add_library(credui SHARED credui_main.c credui.rc)

set_target_properties(credui PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(credui
    ${CMAKE_CURRENT_BINARY_DIR}/credui.def
    wine
    mingw_common
    -ladvapi32
    -luser32
    -lcomctl32)

add_dependencies(credui credui_def psdk)
