
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(xmllite ${CMAKE_CURRENT_SOURCE_DIR}/xmllite.spec ${CMAKE_CURRENT_BINARY_DIR}/xmllite.def)

add_library(xmllite SHARED reader.c xmllite_main.c)

set_module_type(xmllite win32dll)

target_link_libraries(xmllite
    ${CMAKE_CURRENT_BINARY_DIR}/xmllite.def
    wine)

add_importlibs(xmllite kernel32 ntdll)

add_dependencies(xmllite xmllite_def)
add_cab_target(xmllite 1)
