
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(sccbase ${CMAKE_CURRENT_SOURCE_DIR}/sccbase.spec ${CMAKE_CURRENT_BINARY_DIR}/sccbase.def)

add_library(sccbase SHARED main.c)

set_module_type(sccbase win32dll)

target_link_libraries(sccbase
    ${CMAKE_CURRENT_BINARY_DIR}/sccbase.def
    wine)

add_importlibs(sccbase kernel32 ntdll)
add_dependencies(sccbase sccbase_def)
add_cab_target(sccbase 1)
