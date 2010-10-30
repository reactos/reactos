
set_unicode()

list(APPEND SOURCE
    authz.c
    clictx.c
    resman.c
    authz.rc)

spec2def(authz ${CMAKE_CURRENT_SOURCE_DIR}/authz.spec ${CMAKE_CURRENT_BINARY_DIR}/authz.def)

add_library(authz SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/authz_precomp.h.gch
    ${SOURCE})

set_module_type(authz win32dll)

target_link_libraries(authz ${CMAKE_CURRENT_BINARY_DIR}/authz.def)

add_pch(authz ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_importlibs(authz advapi32 kernel32 ntdll)
add_dependencies(authz authz_def)
add_cab_target(authz 1)
