


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
