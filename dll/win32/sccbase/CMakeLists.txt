


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(sccbase ${CMAKE_CURRENT_SOURCE_DIR}/sccbase.spec ${CMAKE_CURRENT_BINARY_DIR}/sccbase.def)

add_library(sccbase SHARED main.c)

set_target_properties(sccbase PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(sccbase
    ${CMAKE_CURRENT_BINARY_DIR}/sccbase.def
    wine
    mingw_common)

add_dependencies(sccbase sccbase_def psdk)
