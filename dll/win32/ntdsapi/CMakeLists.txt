


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(ntdsapi ${CMAKE_CURRENT_SOURCE_DIR}/ntdsapi.spec ${CMAKE_CURRENT_BINARY_DIR}/ntdsapi.def)

add_library(ntdsapi SHARED ntdsapi.c)

set_target_properties(ntdsapi PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(ntdsapi
    ${CMAKE_CURRENT_BINARY_DIR}/ntdsapi.def
    wine
    -luser32
    mingw_common)

add_dependencies(ntdsapi ntdsapi_def psdk)
