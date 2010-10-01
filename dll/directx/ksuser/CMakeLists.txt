
spec2def(ksuser ${CMAKE_CURRENT_SOURCE_DIR}/ksuser.spec ${CMAKE_CURRENT_BINARY_DIR}/ksuser.def)

add_library(ksuser SHARED ksuser.c ksuser.rc)

set_module_type(ksuser win32dll)

target_link_libraries(ksuser ${CMAKE_CURRENT_BINARY_DIR}/ksuser.def)

add_importlibs(ksuser ntdll advapi32)
add_dependencies(ksuser ksuser_def psdk bugcodes)
