
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

add_library(shdoclc MODULE
    rsrc.rc)
spec2def(shdoclc shdoclc.dll shdoclc.spec)

set_module_type(shdoclc win32dll ENTRYPOINT 0)
target_link_libraries(shdoclc wine)
add_importlibs(shdoclc kernel32 ntdll)
add_dependencies(shdoclc psdk)
add_cd_file(TARGET shdoclc DESTINATION reactos/system32 FOR all)
