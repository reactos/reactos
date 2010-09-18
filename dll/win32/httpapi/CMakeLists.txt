


add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(httpapi ${CMAKE_CURRENT_SOURCE_DIR}/httpapi.spec ${CMAKE_CURRENT_BINARY_DIR}/httpapi.def)

add_library(httpapi SHARED httpapi_main.c)

set_target_properties(httpapi PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(httpapi
    ${CMAKE_CURRENT_BINARY_DIR}/httpapi.def
    wine
    mingw_common)

add_dependencies(httpapi httpapi_def psdk)
