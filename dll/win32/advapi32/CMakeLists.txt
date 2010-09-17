
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

add_definitions(-DUNICODE -D_UNICODE)
add_definitions(-D_ADVAPI32_)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_BINARY_DIR}/include/reactos/idl)

file(GLOB_RECURSE SOURCE *.c)

set_source_files_properties(${REACTOS_BINARY_DIR}/include/reactos/idl/svcctl_c.c
    ${REACTOS_BINARY_DIR}/include/reactos/idl/lsa_c.c
    ${REACTOS_BINARY_DIR}/include/reactos/idl/eventlogrpc_c.c
    PROPERTIES GENERATED TRUE)

add_library(advapi32 SHARED
    ${SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/advapi32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/advapi32_advapi32.h.gch
    ${REACTOS_BINARY_DIR}/include/reactos/idl/svcctl_c.c
    ${REACTOS_BINARY_DIR}/include/reactos/idl/lsa_c.c
    ${REACTOS_BINARY_DIR}/include/reactos/idl/eventlogrpc_c.c)

set_target_properties(advapi32 PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(advapi32 ${CMAKE_CURRENT_SOURCE_DIR}/advapi32.def
    wine
    pseh
    rpcrt4
    -lkernel32
    -lntdll)

add_pch(advapi32 ${CMAKE_CURRENT_SOURCE_DIR}/advapi32.h ${SOURCE})
add_dependencies(advapi32 svcctl_client lsa_client eventlogrpc_client version)
