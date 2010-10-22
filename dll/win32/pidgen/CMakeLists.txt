
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(pidgen ${CMAKE_CURRENT_SOURCE_DIR}/pidgen.spec ${CMAKE_CURRENT_BINARY_DIR}/pidgen.def)

add_library(pidgen SHARED main.c rsrc.rc)

set_module_type(pidgen win32dll)

target_link_libraries(pidgen
    ${CMAKE_CURRENT_BINARY_DIR}/pidgen.def
    wine)

add_importlibs(pidgen kernel32 ntdll)
add_dependencies(pidgen pidgen_def)
add_livecd_target(pidgen reactos/system32)
