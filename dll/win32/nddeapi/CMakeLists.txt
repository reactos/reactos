
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(nddeapi ${CMAKE_CURRENT_SOURCE_DIR}/nddeapi.spec ${CMAKE_CURRENT_BINARY_DIR}/nddeapi.def)

add_library(nddeapi SHARED nddeapi.c)

set_target_properties(nddeapi PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(nddeapi
    ${CMAKE_CURRENT_BINARY_DIR}/nddeapi.def
    wine
    mingw_common)

add_dependencies(nddeapi nddeapi_def psdk)
