
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(xmllite.dll xmllite.spec)

add_library(xmllite SHARED reader.c xmllite_main.c ${CMAKE_CURRENT_BINARY_DIR}/xmllite.def)

set_module_type(xmllite win32dll)

target_link_libraries(xmllite
    wine)

add_importlibs(xmllite kernel32 ntdll)


add_cab_target(xmllite 1)
