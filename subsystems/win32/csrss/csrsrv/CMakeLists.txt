
include_directories(${REACTOS_SOURCE_DIR}/subsystems/win32/csrss/include)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

spec2def(csrsrv ${CMAKE_CURRENT_SOURCE_DIR}/csrsrv.spec ${CMAKE_CURRENT_BINARY_DIR}/csrsrv.def)

list(APPEND SOURCE
    api/process.c
    api/user.c
    api/wapi.c
    procsup.c
    thredsup.c
    init.c)

add_library(csrsrv SHARED ${CMAKE_CURRENT_BINARY_DIR}/csrsrv_srv.h.gch ${SOURCE})

target_link_libraries(csrsrv
    ${CMAKE_CURRENT_BINARY_DIR}/csrsrv.def
    pseh)

set_module_type(csrsrv nativedll)

add_importlibs(csrsrv ntdll smdll)

add_pch(csrsrv ${CMAKE_CURRENT_SOURCE_DIR}/srv.h ${SOURCE})

add_dependencies(csrsrv csrsrv_def psdk bugcodes)
