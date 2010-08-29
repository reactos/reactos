
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(sxs ${CMAKE_CURRENT_SOURCE_DIR}/sxs.spec ${CMAKE_CURRENT_BINARY_DIR}/sxs.def)

add_library(sxs SHARED sxs.c)

set_target_properties(sxs PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(sxs
    ${CMAKE_CURRENT_BINARY_DIR}/sxs.def
    wine
    mingw_common)

add_dependencies(sxs sxs_def psdk)
