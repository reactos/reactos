
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(usp10 ${CMAKE_CURRENT_SOURCE_DIR}/usp10.spec ${CMAKE_CURRENT_BINARY_DIR}/usp10.def)

add_library(usp10 SHARED
    bidi.c
    usp10.c
    mirror.c
    shape.c
    shaping.c)

set_target_properties(usp10 PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(usp10
    ${CMAKE_CURRENT_BINARY_DIR}/usp10.def
    wine
    -lgdi32
    mingw_common)

add_dependencies(usp10 usp10_def psdk)
