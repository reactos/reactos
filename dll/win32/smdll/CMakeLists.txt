
spec2def(smdll.dll smdll.spec)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

add_library(smdll SHARED
    dllmain.c
    query.c
    smdll.rc
    ${CMAKE_CURRENT_BINARY_DIR}/smdll.def)

target_link_libraries(smdll smlib)

add_importlibs(smdll ntdll)
add_dependencies(smdll psdk buildno_header)
add_cab_target(smdll 1)
add_importlib_target(smdll.spec)
