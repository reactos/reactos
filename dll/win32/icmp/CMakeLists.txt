
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

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
