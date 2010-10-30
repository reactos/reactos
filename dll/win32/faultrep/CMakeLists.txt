
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(faultrep.dll faultrep.spec)

add_library(faultrep SHARED faultrep.c ${CMAKE_CURRENT_BINARY_DIR}/faultrep.def)

set_module_type(faultrep win32dll)

target_link_libraries(faultrep wine)

add_importlibs(faultrep advapi32 kernel32 ntdll)

add_cab_target(faultrep 1)
