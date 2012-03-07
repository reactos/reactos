
include_directories(${REACTOS_SOURCE_DIR}/subsystems/win32/csrss/include)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

spec2def(csrsrv.dll csrsrv.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    api/process.c
    api/user.c
    api/wapi.c
    procsup.c
    thredsup.c
    init.c
    wait.c
    session.c
    server.c
    ${CMAKE_CURRENT_BINARY_DIR}/csrsrv.def)

add_library(csrsrv SHARED ${SOURCE})

target_link_libraries(csrsrv ${PSEH_LIB} smlib)

set_module_type(csrsrv nativedll)

add_importlibs(csrsrv ntdll smdll)

add_pch(csrsrv srv.h)

add_dependencies(csrsrv psdk bugcodes)
add_cd_file(TARGET csrsrv DESTINATION reactos/system32 FOR all)

