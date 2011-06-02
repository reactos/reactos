
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

list(APPEND SOURCE
    client.c
    debug.c
    init.c
    initdosdev.c
    initenv.c
    initheap.c
    initmv.c
    initobdir.c
    initpage.c
    initreg.c
    initrun.c
    initss.c
    initwkdll.c
    print.c
    smapi.c
    smapicomp.c
    smapiexec.c
    smapiquery.c
    smss.c
    smss.rc)

add_executable(smss WIN32 ${SOURCE})

target_link_libraries(smss nt smlib)
if(MSVC)
    target_link_libraries(smss msvcsup)
    set_entrypoint(smss DllMainCRTStartup)
endif()

add_pch(smss smss.h)

set_module_type(smss nativecui)
add_importlibs(smss ntdll)
add_cd_file(TARGET smss DESTINATION reactos/system32 FOR all)
