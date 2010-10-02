


add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(activeds ${CMAKE_CURRENT_SOURCE_DIR}/activeds.spec ${CMAKE_CURRENT_BINARY_DIR}/activeds.def)

add_library(activeds SHARED activeds_main.c)

set_target_properties(activeds PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(activeds 
     ${CMAKE_CURRENT_BINARY_DIR}/activeds.def
     wine
     mingw_common)

add_dependencies(activeds activeds_def psdk buildno_header)
