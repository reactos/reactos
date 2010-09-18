


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(pidgen ${CMAKE_CURRENT_SOURCE_DIR}/pidgen.spec ${CMAKE_CURRENT_BINARY_DIR}/pidgen.def)

add_library(pidgen SHARED main.c rsrc.rc)

set_target_properties(pidgen PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(pidgen
    ${CMAKE_CURRENT_BINARY_DIR}/pidgen.def
    wine
    mingw_common)

add_dependencies(pidgen pidgen_def psdk)
