
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mcicda ${CMAKE_CURRENT_SOURCE_DIR}/mcicda.spec ${CMAKE_CURRENT_BINARY_DIR}/mcicda.def)

add_library(mcicda SHARED mcicda.c)

set_target_properties(mcicda PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(mcicda
    ${CMAKE_CURRENT_BINARY_DIR}/mcicda.def
    wine
    -lwinmm
    mingw_common
    -luser32)

add_dependencies(mcicda mcicda_def psdk)
