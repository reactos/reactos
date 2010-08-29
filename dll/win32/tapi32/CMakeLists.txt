
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(tapi32 ${CMAKE_CURRENT_SOURCE_DIR}/tapi32.spec ${CMAKE_CURRENT_BINARY_DIR}/tapi32.def)

add_library(tapi32 SHARED
    assisted.c
    internal.c
    line.c
    phone.c)

set_target_properties(tapi32 PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(tapi32
    ${CMAKE_CURRENT_BINARY_DIR}/tapi32.def
    wine
    -ladvapi32
    mingw_common)

add_dependencies(tapi32 tapi32_def psdk)
