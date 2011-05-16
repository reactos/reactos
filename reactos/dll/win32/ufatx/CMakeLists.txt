
spec2def(ufatx.dll ufatx.spec)

list(APPEND SOURCE
    ufatx.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ufatx.def)

add_library(ufatx SHARED ${SOURCE})

set_entrypoint(ufatx 0)

target_link_libraries(ufatx vfatxlib -lgcc)
add_importlibs(ufatx kernel32 ntdll)
add_importlib_target(ufatx.spec)
add_dependencies(ufatx psdk)
add_cd_file(TARGET ufatx DESTINATION reactos/system32 FOR all)
