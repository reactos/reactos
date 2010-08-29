
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(updspapi ${CMAKE_CURRENT_SOURCE_DIR}/updspapi.spec ${CMAKE_CURRENT_BINARY_DIR}/updspapi.def)

add_library(updspapi SHARED main.c)

set_target_properties(updspapi PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(updspapi
    ${CMAKE_CURRENT_BINARY_DIR}/updspapi.def
    wine
    -lsetupapi
    mingw_common)

add_dependencies(updspapi updspapi_def psdk)
