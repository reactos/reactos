


add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(clusapi ${CMAKE_CURRENT_SOURCE_DIR}/clusapi.spec ${CMAKE_CURRENT_BINARY_DIR}/clusapi.def)

add_library(clusapi SHARED clusapi.c)

set_target_properties(clusapi PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(clusapi
    ${CMAKE_CURRENT_BINARY_DIR}/clusapi.def
    wine
    mingw_common)

add_dependencies(clusapi clusapi_def psdk)
