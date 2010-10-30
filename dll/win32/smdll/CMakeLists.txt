
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

spec2def(smdll.dll smdll.spec)

add_library(smdll SHARED
    dllmain.c
    query.c
    smdll.rc
    ${CMAKE_CURRENT_BINARY_DIR}/smdll.def)

target_link_libraries(smdll smlib)

add_importlibs(smdll ntdll)
add_dependencies(smdll psdk buildno_header)
add_cab_target(smdll 1)