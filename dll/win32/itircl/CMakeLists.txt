
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(itircl ${CMAKE_CURRENT_SOURCE_DIR}/itircl.spec ${CMAKE_CURRENT_BINARY_DIR}/itircl.def)

add_library(itircl SHARED itircl_main.c)

set_target_properties(itircl PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(itircl
    ${CMAKE_CURRENT_BINARY_DIR}/itircl.def
    wine
    mingw_common)

add_dependencies(itircl itircl_def psdk)
