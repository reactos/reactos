
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(activeds ${CMAKE_CURRENT_SOURCE_DIR}/activeds.spec ${CMAKE_CURRENT_BINARY_DIR}/activeds.def)

add_library(activeds SHARED activeds_main.c)

set_target_properties(activeds PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(activeds 
     ${CMAKE_CURRENT_BINARY_DIR}/activeds.def
     wine
     mingw_common)

add_dependencies(activeds activeds_def psdk)
