
include_directories(include)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/drivers)

add_executable(csrss csrss.c csrss.rc)

set_module_type(csrss nativecui)

target_link_libraries(csrss nt)

add_importlibs(csrss ntdll csrsrv)

add_dependencies(csrss psdk bugcodes)

add_subdirectory(csrsrv)
add_subdirectory(win32csr)
