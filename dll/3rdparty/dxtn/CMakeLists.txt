
add_definitions(-D_DLL -D__USE_CRTIMP)

spec2def(dxtn ${CMAKE_CURRENT_SOURCE_DIR}/dxtn.spec ${CMAKE_CURRENT_BINARY_DIR}/dxtn.def)

add_library(dxtn SHARED
    fxt1.c
    dxtn.c
    wrapper.c
    texstore.c)

set_entrypoint(dxtn 0)

target_link_libraries(dxtn ${CMAKE_CURRENT_BINARY_DIR}/dxtn.def)

add_importlibs(dxtn msvcrt)

add_dependencies(dxtn dxtn_def psdk buildno_header)
