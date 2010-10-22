
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(faultrep ${CMAKE_CURRENT_SOURCE_DIR}/faultrep.spec ${CMAKE_CURRENT_BINARY_DIR}/faultrep.def)

add_library(faultrep SHARED faultrep.c)

set_module_type(faultrep win32dll)

target_link_libraries(faultrep
    ${CMAKE_CURRENT_BINARY_DIR}/faultrep.def
    wine)

add_importlibs(faultrep advapi32 kernel32 ntdll)
add_dependencies(faultrep faultrep_def)
add_livecd_target(faultrep reactos/system32)
