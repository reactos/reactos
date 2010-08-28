
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(olethk32 ${CMAKE_CURRENT_SOURCE_DIR}/olethk32.spec ${CMAKE_CURRENT_BINARY_DIR}/olethk32.def)

add_library(olethk32 SHARED main.c version.rc)

set_target_properties(olethk32 PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(olethk32
    ${CMAKE_CURRENT_BINARY_DIR}/olethk32.def
    wine
    mingw_common)

add_dependencies(olethk32 olethk32_def psdk)
