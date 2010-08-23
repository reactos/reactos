
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mciseq ${CMAKE_CURRENT_SOURCE_DIR}/mciseq.spec ${CMAKE_CURRENT_BINARY_DIR}/mciseq.def)

add_library(mciseq SHARED mcimidi.c)

set_target_properties(mciseq PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(mciseq
    ${CMAKE_CURRENT_BINARY_DIR}/mciseq.def
    wine
    -lwinmm
    mingw_common
    ${REACTOS_SOURCE_DIR}/dll/win32/user32/libuser32.a)

add_dependencies(mciseq mciseq_def psdk)
