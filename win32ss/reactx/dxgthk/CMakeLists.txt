
list(APPEND SOURCE
    main.c
    dxgthk.rc)

add_library(dxgthk MODULE ${SOURCE})
spec2def(dxgthk dxgthk.sys dxgthk.spec ADD_IMPORTLIB)

set_module_type(dxgthk kernelmodedriver)
add_importlibs(dxgthk win32k)
add_cd_file(TARGET dxgthk DESTINATION reactos/system32/drivers FOR all)
