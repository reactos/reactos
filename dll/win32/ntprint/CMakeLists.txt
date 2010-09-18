


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(ntprint ${CMAKE_CURRENT_SOURCE_DIR}/ntprint.spec ${CMAKE_CURRENT_BINARY_DIR}/ntprint.def)

add_library(ntprint SHARED ntprint.c ntprint.rc)

set_target_properties(ntprint PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(ntprint
    ${CMAKE_CURRENT_BINARY_DIR}/ntprint.def
    wine
    -lwinspool
    mingw_common)

add_dependencies(ntprint ntprint_def psdk)
