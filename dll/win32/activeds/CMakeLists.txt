
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(activeds.dll activeds.spec)

add_library(activeds SHARED activeds_main.c ${CMAKE_CURRENT_BINARY_DIR}/activeds.def)

set_module_type(activeds win32dll)

target_link_libraries(activeds wine)

add_importlibs(activeds kernel32 ntdll)


add_cab_target(activeds 1)
