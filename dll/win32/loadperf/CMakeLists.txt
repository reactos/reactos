


add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(loadperf ${CMAKE_CURRENT_SOURCE_DIR}/loadperf.spec ${CMAKE_CURRENT_BINARY_DIR}/loadperf.def)

add_library(loadperf SHARED loadperf_main.c)

set_target_properties(loadperf PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(loadperf
    ${CMAKE_CURRENT_BINARY_DIR}/loadperf.def
    wine
    mingw_common)

add_dependencies(loadperf loadperf_def psdk)
