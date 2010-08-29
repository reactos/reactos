
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

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
