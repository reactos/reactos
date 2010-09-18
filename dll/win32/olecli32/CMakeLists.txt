


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(olecli32 ${CMAKE_CURRENT_SOURCE_DIR}/olecli32.spec ${CMAKE_CURRENT_BINARY_DIR}/olecli32.def)

add_library(olecli32 SHARED olecli_main.c)

set_target_properties(olecli32 PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(olecli32
    ${CMAKE_CURRENT_BINARY_DIR}/olecli32.def
    wine
    -lole32
    -lgdi32
    mingw_common)

add_dependencies(olecli32 olecli32_def psdk)
