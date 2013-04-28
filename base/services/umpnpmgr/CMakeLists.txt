
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/idl)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_rpc_files(server ${REACTOS_SOURCE_DIR}/include/reactos/idl/pnp.idl)


add_executable(umpnpmgr
    umpnpmgr.c
    umpnpmgr.rc
    ${CMAKE_CURRENT_BINARY_DIR}/pnp_s.c)

target_link_libraries(umpnpmgr
    wdmguid
    ${PSEH_LIB})

set_module_type(umpnpmgr win32cui UNICODE)
add_importlibs(umpnpmgr advapi32 rpcrt4 userenv msvcrt kernel32 ntdll)
add_cd_file(TARGET umpnpmgr DESTINATION reactos/system32 FOR all)
