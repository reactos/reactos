
spec2def(psapi ${CMAKE_CURRENT_SOURCE_DIR}/psapi.spec ${CMAKE_CURRENT_BINARY_DIR}/psapi.def)

list(APPEND SOURCE
    malloc.c
    psapi.c
    psapi.rc)

add_library(psapi SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/psapi_precomp.h.gch
    ${SOURCE})

set_module_type(psapi win32dll)

target_link_libraries(psapi
    ${CMAKE_CURRENT_BINARY_DIR}/psapi.def
    epsapi
    pseh)

add_importlibs(psapi kernel32 ntdll)
add_pch(psapi ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(psapi psapi_def)
