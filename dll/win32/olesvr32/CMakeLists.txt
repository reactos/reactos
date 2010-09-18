


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(olesvr32 ${CMAKE_CURRENT_SOURCE_DIR}/olesvr32.spec ${CMAKE_CURRENT_BINARY_DIR}/olesvr32.def)

add_library(olesvr32 SHARED olesvr_main.c)

set_target_properties(olesvr32 PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(olesvr32
    ${CMAKE_CURRENT_BINARY_DIR}/olesvr32.def
    wine
    mingw_common)

add_dependencies(olesvr32 olesvr32_def psdk)
