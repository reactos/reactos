
spec2def(dxgthk.sys dxgthk.spec)

add_library(dxgthk SHARED
    main.c
    dxgthk.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dxgthk.def)

set_module_type(dxgthk kernelmodedriver)
add_importlibs(dxgthk win32k)
