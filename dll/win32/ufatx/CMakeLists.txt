
spec2def(ufatx ${CMAKE_CURRENT_SOURCE_DIR}/ufatx.spec ${CMAKE_CURRENT_BINARY_DIR}/ufatx.def)

add_library(ufatx SHARED ufatx.rc)

set_entrypoint(ufatx 0)

target_link_libraries(ufatx
    ${CMAKE_CURRENT_BINARY_DIR}/ufatx.def
    vfatxlib)

add_importlibs(ufatx gcc kernel32 ntdll)
add_dependencies(ufatx ufatx_def psdk buildno_header)
