
spec2def(d3d8thk.dll d3d8thk.spec)

add_library(d3d8thk MODULE
    d3d8thk.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3d8thk.def)

set_entrypoint(d3d8thk 0)
add_importlibs(d3d8thk gdi32)
add_dependencies(d3d8thk psdk)
add_cd_file(TARGET d3d8thk DESTINATION reactos/system32 FOR all)
