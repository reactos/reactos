


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(traffic ${CMAKE_CURRENT_SOURCE_DIR}/traffic.spec ${CMAKE_CURRENT_BINARY_DIR}/traffic.def)

add_library(traffic SHARED traffic_main.c)

set_target_properties(traffic PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(traffic
    ${CMAKE_CURRENT_BINARY_DIR}/traffic.def
    wine
    mingw_common)

add_dependencies(traffic traffic_def psdk)
