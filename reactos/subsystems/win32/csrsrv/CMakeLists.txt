
include_directories(${REACTOS_SOURCE_DIR}/subsystems/win32/csrss/include)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

spec2def(csrsrv.dll csrsrv.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    api.c
    init.c
    procsup.c
    server.c
    session.c
    thredsup.c
    wait.c
    srv.h)

add_library(csrsrv SHARED
    ${SOURCE}
    csrsrv.rc
    ${CMAKE_CURRENT_BINARY_DIR}/csrsrv.def)

set_module_type(csrsrv nativedll)
target_link_libraries(csrsrv ${PSEH_LIB} smlib)
add_importlibs(csrsrv ntdll smdll)
add_pch(csrsrv srv.h SOURCE)
add_dependencies(csrsrv psdk bugcodes)
add_cd_file(TARGET csrsrv DESTINATION reactos/system32 FOR all)
