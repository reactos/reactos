


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(icmp ${CMAKE_CURRENT_SOURCE_DIR}/icmp.spec ${CMAKE_CURRENT_BINARY_DIR}/icmp.def)

add_library(icmp SHARED icmp_main.c icmp.rc)

set_target_properties(icmp PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(icmp
    ${CMAKE_CURRENT_BINARY_DIR}/icmp.def
    wine
    -lws2_32
    mingw_common)

add_dependencies(icmp icmp_def psdk)
