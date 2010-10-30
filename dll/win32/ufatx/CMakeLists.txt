
spec2def(ufatx.dll ufatx.spec)

add_library(ufatx SHARED ufatx.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ufatx.def)

set_entrypoint(ufatx 0)

target_link_libraries(ufatx vfatxlib)

add_importlibs(ufatx gcc kernel32 ntdll)
add_dependencies(ufatx psdk buildno_header)
add_cab_target(ufatx 1)