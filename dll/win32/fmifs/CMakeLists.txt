


spec2def(fmifs ${CMAKE_CURRENT_SOURCE_DIR}/fmifs.spec ${CMAKE_CURRENT_BINARY_DIR}/fmifs.def)

list(APPEND SOURCE
    chkdsk.c
    compress.c
    diskcopy.c
    extend.c
    format.c
    init.c
    media.c
    query.c
    fmifs.rc)

add_library(fmifs SHARED ${SOURCE})

set_target_properties(fmifs PROPERTIES LINK_FLAGS "-Wl,-entry,_InitializeFmIfs@12")

target_link_libraries(fmifs
    ${CMAKE_CURRENT_BINARY_DIR}/fmifs.def
    -lkernel32
    -lntdll)

add_pch(fmifs ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(fmifs fmifs_def psdk)
