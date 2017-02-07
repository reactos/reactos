
include_directories(${REACTOS_SOURCE_DIR}/subsystems/win32/csrss/include)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

spec2def(csrsrv2.dll csrsrv2.spec ADD_IMPORTLIB)

list(APPEND SOURCE
     api.c
     init.c
     process.c
     server.c
     session.c
     thread.c
     wait.c
     csrsrv.rc
     ${CMAKE_CURRENT_BINARY_DIR}/csrsrv2.def)

add_library(csrsrv2 SHARED ${SOURCE})

target_link_libraries(csrsrv2 ${PSEH_LIB} smlib)

set_module_type(csrsrv2 nativedll)

add_importlibs(csrsrv2 ntdll)

add_pch(csrsrv2 srv.h)

add_dependencies(csrsrv2 psdk bugcodes)
add_cd_file(TARGET csrsrv2 DESTINATION reactos/system32 FOR all)

