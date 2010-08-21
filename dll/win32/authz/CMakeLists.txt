
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

add_definitions(-DUNICODE -D_UNICODE)

file(GLOB_RECURSE SOURCE *.c)

spec2def(authz ${CMAKE_CURRENT_SOURCE_DIR}/authz.spec ${CMAKE_CURRENT_BINARY_DIR}/authz.def)

add_library(authz SHARED
    ${SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/authz.rc
    ${CMAKE_CURRENT_BINARY_DIR}/authz_precomp.h.gch)

set_target_properties(authz PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(authz
    ${CMAKE_CURRENT_BINARY_DIR}/authz.def
    ${REACTOS_SOURCE_DIR}/dll/win32/advapi32/libadvapi32.a
    ${REACTOS_SOURCE_DIR}/dll/win32/kernel32/libkernel32.a
    ${REACTOS_SOURCE_DIR}/dll/ntdll/libntdll.a)

add_pch(authz ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(authz authz_def psdk)
