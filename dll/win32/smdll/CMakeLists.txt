
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

spec2def(smdll.dll smdll.spec)

list(APPEND SOURCE
    dllmain.c
    query.c
    smdll.rc
    ${CMAKE_CURRENT_BINARY_DIR}/smdll.def)

add_library(smdll SHARED ${SOURCE})

target_link_libraries(smdll smlib)
add_importlibs(smdll ntdll)
add_dependencies(smdll psdk)
add_cab_target(smdll 1)
add_importlib_target(smdll.spec)
