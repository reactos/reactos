
add_definitions(-D__SECUR32__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

spec2def(secur32 ${CMAKE_CURRENT_SOURCE_DIR}/secur32.spec ${CMAKE_CURRENT_BINARY_DIR}/secur32.def)

list(APPEND SOURCE
    dllmain.c
    secext.c
    sspi.c
    secur32.rc)

add_library(secur32 SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/secur32_precomp.h.gch
    ${SOURCE})

set_module_type(secur32 win32dll)

target_link_libraries(secur32
    ${CMAKE_CURRENT_BINARY_DIR}/secur32.def
    lsalib)

add_importlibs(secur32 advapi32 kernel32 ntdll)
add_pch(secur32 ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(secur32 secur32_def)
add_cab_target(secur32 1)
