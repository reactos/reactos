
spec2def(d3d8thk ${CMAKE_CURRENT_SOURCE_DIR}/d3d8thk.spec ${CMAKE_CURRENT_BINARY_DIR}/d3d8thk.def)

add_library(d3d8thk SHARED d3d8thk.rc)

set_entrypoint(d3d8thk 0)

target_link_libraries(d3d8thk ${CMAKE_CURRENT_BINARY_DIR}/d3d8thk.def)

add_importlibs(d3d8thk gdi32)

add_dependencies(d3d8thk d3d8thk_def psdk buildno_header)
