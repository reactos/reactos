
add_definitions(-D_DXAPI_)

add_library(dxapi MODULE
    main.c
    dxapi.rc)
spec2def(dxapi dxapi.sys dxapi.spec)

set_module_type(dxapi kernelmodedriver)
add_importlibs(dxapi ntoskrnl hal videoprt)
add_cd_file(TARGET dxapi DESTINATION reactos/system32/drivers FOR all)
