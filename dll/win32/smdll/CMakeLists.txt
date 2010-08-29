
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

spec2def(smdll ${CMAKE_CURRENT_SOURCE_DIR}/smdll.spec ${CMAKE_CURRENT_BINARY_DIR}/smdll.def)

add_library(smdll SHARED
    dllmain.c
    query.c
    smdll.rc)

target_link_libraries(smdll
    ${CMAKE_CURRENT_BINARY_DIR}/smdll.def
    smlib
    ${REACTOS_SOURCE_DIR}/dll/ntdll/libntdll.a)

add_dependencies(smdll smdll_def psdk)
