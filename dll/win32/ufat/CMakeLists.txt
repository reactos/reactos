
spec2def(ufat ${CMAKE_CURRENT_SOURCE_DIR}/ufat.spec ${CMAKE_CURRENT_BINARY_DIR}/ufat.def)

add_library(ufat SHARED ufat.c ufat.rc)

set_module_type(ufat win32dll)

target_link_libraries(ufat
    ${CMAKE_CURRENT_BINARY_DIR}/ufat.def
    vfatlib)

add_importlibs(ufat kernel32 ntdll)
add_dependencies(ufat ufat_def)
add_cab_target(ufat 1)
