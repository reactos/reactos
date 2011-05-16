
add_definitions(-D__SECUR32__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

spec2def(secur32.dll secur32.spec)

list(APPEND SOURCE
    dllmain.c
    secext.c
    sspi.c
    secur32.rc
    stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/secur32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/secur32.def
)

add_library(secur32 SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/secur32_precomp.h.gch
    ${SOURCE})

set_module_type(secur32 win32dll)

target_link_libraries(secur32 lsalib)

add_importlibs(secur32 advapi32 msvcrt kernel32 ntdll)
add_pch(secur32 ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})

add_cd_file(TARGET secur32 DESTINATION reactos/system32 FOR all)
add_importlib_target(secur32.spec)
