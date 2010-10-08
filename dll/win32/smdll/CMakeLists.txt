
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

spec2def(smdll ${CMAKE_CURRENT_SOURCE_DIR}/smdll.spec ${CMAKE_CURRENT_BINARY_DIR}/smdll.def)

add_library(smdll SHARED
    dllmain.c
    query.c
    smdll.rc)

target_link_libraries(smdll
    ${CMAKE_CURRENT_BINARY_DIR}/smdll.def
    smlib)

add_importlibs(smdll ntdll)
add_dependencies(smdll smdll_def psdk buildno_header)
