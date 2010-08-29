
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(sensapi ${CMAKE_CURRENT_SOURCE_DIR}/sensapi.spec ${CMAKE_CURRENT_BINARY_DIR}/sensapi.def)

add_library(sensapi SHARED sensapi.c)

set_target_properties(sensapi PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(sensapi
    ${CMAKE_CURRENT_BINARY_DIR}/sensapi.def
    wine
    mingw_common)

add_dependencies(sensapi sensapi_def psdk)
