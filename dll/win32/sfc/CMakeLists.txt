
spec2def(sfc ${CMAKE_CURRENT_SOURCE_DIR}/sfc.spec ${CMAKE_CURRENT_BINARY_DIR}/sfc.def)

list(APPEND SOURCE sfc.c)

add_library(sfc SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/sfc_precomp.h.gch
    ${SOURCE})

set_module_type(sfc win32dll)

target_link_libraries(sfc ${CMAKE_CURRENT_BINARY_DIR}/sfc.def)

add_importlibs(sfc kernel32)
add_pch(sfc ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(sfc sfc_def)
