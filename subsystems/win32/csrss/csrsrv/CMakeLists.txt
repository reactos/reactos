
include_directories(${REACTOS_SOURCE_DIR}/subsystems/win32/csrss/include)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

spec2def(csrsrv.dll csrsrv.spec)

list(APPEND SOURCE
    api/process.c
    api/user.c
    api/wapi.c
    procsup.c
    thredsup.c
    init.c
    ${CMAKE_CURRENT_BINARY_DIR}/csrsrv.def)

add_library(csrsrv SHARED ${CMAKE_CURRENT_BINARY_DIR}/csrsrv_srv.h.gch ${SOURCE})

target_link_libraries(csrsrv ${PSEH_LIB})

set_module_type(csrsrv nativedll)

if(MSVC)
set_entrypoint(csrsrv DllMainCRTStartup)
endif()


add_importlibs(csrsrv ntdll smdll)

add_pch(csrsrv ${CMAKE_CURRENT_SOURCE_DIR}/srv.h ${SOURCE})

add_dependencies(csrsrv psdk bugcodes)
add_cd_file(TARGET csrsrv DESTINATION reactos/system32 FOR all)
add_importlib_target(csrsrv.spec)
