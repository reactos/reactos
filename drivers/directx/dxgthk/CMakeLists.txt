
spec2def(dxgthk.sys dxgthk.spec)

list(APPEND SOURCE
    main.c
    dxgthk.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dxgthk.def)

add_library(dxgthk SHARED ${SOURCE})

set_module_type(dxgthk kernelmodedriver)

add_importlibs(dxgthk win32k)
add_importlib_target(dxgthk.spec dxgthk.sys)

add_cab_target(dxgthk 2)
