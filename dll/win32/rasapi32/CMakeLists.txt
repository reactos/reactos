


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(rasapi32 ${CMAKE_CURRENT_SOURCE_DIR}/rasapi32.spec ${CMAKE_CURRENT_BINARY_DIR}/rasapi32.def)

add_library(rasapi32 SHARED rasapi.c)

set_target_properties(rasapi32 PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(rasapi32
    ${CMAKE_CURRENT_BINARY_DIR}/rasapi32.def
    wine
    mingw_common)

add_dependencies(rasapi32 rasapi32_def psdk)
