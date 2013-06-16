
spec2def(ufatx.dll ufatx.spec)

list(APPEND SOURCE
    ufatx.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ufatx.def)

add_library(ufatx SHARED ${SOURCE})

set_module_type(ufatx win32dll ENTRYPOINT 0 )

target_link_libraries(ufatx vfatxlib)
add_importlibs(ufatx ntdll)

add_dependencies(ufatx psdk)
add_cd_file(TARGET ufatx DESTINATION reactos/system32 FOR all)
