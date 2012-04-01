
include_directories(
    include
    ${REACTOS_SOURCE_DIR}/include/reactos/subsys
    ${REACTOS_SOURCE_DIR}/include/reactos/drivers)

add_executable(csrss csrss.c csrss.rc)

set_module_type(csrss nativecui)
target_link_libraries(csrss nt)
add_importlibs(csrss ntdll csrsrv)
add_dependencies(csrss psdk bugcodes)
add_cd_file(TARGET csrss DESTINATION reactos/system32 FOR all)

add_subdirectory(csrsrv)
