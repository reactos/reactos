
add_definitions(-D_DXAPI_)
spec2def(dxapi.sys dxapi.spec)

add_library(dxapi SHARED
    main.c
    dxapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dxapi.def)

set_module_type(dxapi kernelmodedriver)
add_importlibs(dxapi ntoskrnl hal videoprt)
add_cd_file(TARGET dxapi DESTINATION reactos/system32/drivers FOR all)
