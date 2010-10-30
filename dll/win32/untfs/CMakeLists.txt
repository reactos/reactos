
spec2def(untfs ${CMAKE_CURRENT_SOURCE_DIR}/untfs.spec ${CMAKE_CURRENT_BINARY_DIR}/untfs.def)

add_library(untfs SHARED untfs.c untfs.rc)

set_module_type(untfs win32dll)

target_link_libraries(untfs
    ${CMAKE_CURRENT_BINARY_DIR}/untfs.def
    ntfslib)

add_importlibs(untfs kernel32 ntdll)
add_dependencies(untfs untfs_def)
add_cab_target(untfs 1)
