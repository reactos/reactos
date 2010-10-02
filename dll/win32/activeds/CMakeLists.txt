
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(activeds ${CMAKE_CURRENT_SOURCE_DIR}/activeds.spec ${CMAKE_CURRENT_BINARY_DIR}/activeds.def)

add_library(activeds SHARED activeds_main.c)

set_module_type(activeds win32dll)

target_link_libraries(activeds 
     ${CMAKE_CURRENT_BINARY_DIR}/activeds.def
     wine)

add_importlibs(activeds kernel32 ntdll)

add_dependencies(activeds activeds_def)
