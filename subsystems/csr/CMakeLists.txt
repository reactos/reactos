
include_directories(
    include
    ${REACTOS_SOURCE_DIR}/include/reactos/subsys
    ${REACTOS_SOURCE_DIR}/include/reactos/drivers)

add_executable(csrss2 main.c csr.rc)

set_module_type(csrss2 nativecui)
target_link_libraries(csrss2 nt)
add_importlibs(csrss2 ntdll csrsrv2)
add_dependencies(csrss2 psdk bugcodes)
add_cd_file(TARGET csrss2 DESTINATION reactos/system32 FOR all)

add_subdirectory(csrsrv)

