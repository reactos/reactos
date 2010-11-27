
list(APPEND SOURCE
    malloc.c
    psapi.c
    psapi.rc
    psapi.def)

if(NOT MSVC)
    set_source_files_properties(psapi.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

add_library(psapi SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/psapi_precomp.h.gch
    ${SOURCE})

set_module_type(psapi win32dll)

target_link_libraries(psapi
    epsapi
    ${PSEH_LIB})

add_importlibs(psapi msvcrt kernel32 ntdll)
add_pch(psapi ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})

add_cab_target(psapi 1)
