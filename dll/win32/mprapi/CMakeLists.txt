


add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mprapi ${CMAKE_CURRENT_SOURCE_DIR}/mprapi.spec ${CMAKE_CURRENT_BINARY_DIR}/mprapi.def)

add_library(mprapi SHARED mprapi.c)

set_target_properties(mprapi PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(mprapi
    ${CMAKE_CURRENT_BINARY_DIR}/mprapi.def
    wine
    mingw_common)

add_dependencies(mprapi mprapi_def psdk)
