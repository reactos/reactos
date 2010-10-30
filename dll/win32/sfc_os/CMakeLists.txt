
spec2def(sfc_os ${CMAKE_CURRENT_SOURCE_DIR}/sfc_os.spec ${CMAKE_CURRENT_BINARY_DIR}/sfc_os.def)

list(APPEND SOURCE sfc_os.c)

add_library(sfc_os SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/sfc_os_precomp.h.gch
    ${SOURCE})

set_module_type(sfc_os win32dll)

target_link_libraries(sfc_os ${CMAKE_CURRENT_BINARY_DIR}/sfc_os.def)

add_importlibs(sfc_os kernel32 ntdll)
add_pch(sfc_os ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(sfc_os sfc_os_def)
add_cab_target(sfc_os 1)
