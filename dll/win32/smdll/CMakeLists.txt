
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys)
spec2def(smdll.dll smdll.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    dllmain.c
    query.c
    precomp.h)

add_library(smdll MODULE
    ${SOURCE}
    smdll.rc
    ${CMAKE_CURRENT_BINARY_DIR}/smdll.def)

target_link_libraries(smdll smlib)
set_module_type(smdll nativedll ENTRYPOINT DllMainCRTStartup 12)
add_importlibs(smdll ntdll)
add_pch(smdll precomp.h SOURCE)
add_dependencies(smdll psdk)
add_cd_file(TARGET smdll DESTINATION reactos/system32 FOR all)
