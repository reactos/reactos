
spec2def(psapi.dll psapi.spec)

list(APPEND SOURCE
    malloc.c
    psapi.c
    psapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/psapi.def)

add_library(psapi SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/psapi_precomp.h.gch
    ${SOURCE})

set_module_type(psapi win32dll)
target_link_libraries(psapi epsapi ${PSEH_LIB})
add_importlibs(psapi msvcrt kernel32 ntdll)
add_importlib_target(psapi.spec)
add_pch(psapi ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_cd_file(TARGET psapi DESTINATION reactos/system32 FOR all)
