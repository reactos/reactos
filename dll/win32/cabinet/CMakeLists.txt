
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(cabinet ${CMAKE_CURRENT_SOURCE_DIR}/cabinet.spec ${CMAKE_CURRENT_BINARY_DIR}/cabinet.def)

file(GLOB_RECURSE SOURCE *.c)

add_library(cabinet SHARED
    ${SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/cabinet.rc)

set_target_properties(cabinet PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(cabinet
    ${CMAKE_CURRENT_BINARY_DIR}/cabinet.def
    wine
    mingw_common)

add_dependencies(cabinet cabinet_def psdk)
