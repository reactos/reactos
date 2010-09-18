


add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(faultrep ${CMAKE_CURRENT_SOURCE_DIR}/faultrep.spec ${CMAKE_CURRENT_BINARY_DIR}/faultrep.def)

add_library(faultrep SHARED faultrep.c)

set_target_properties(faultrep PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(faultrep
    ${CMAKE_CURRENT_BINARY_DIR}/faultrep.def
    wine
    mingw_common
    -ladvapi32)

add_dependencies(faultrep faultrep_def psdk)
