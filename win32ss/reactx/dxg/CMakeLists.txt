
spec2def(dxg.sys dxg.spec)

list(APPEND SOURCE
    main.c
    ddhmg.c
    eng.c
    historic.c
    dxg_int.h)

add_library(dxg SHARED
    ${SOURCE}
    dxg.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dxg.def)

set_module_type(dxg kernelmodedriver)
add_importlibs(dxg dxgthk ntoskrnl)
add_pch(dxg dxg_int.h SOURCE)
add_cd_file(TARGET dxg DESTINATION reactos/system32/drivers FOR all)
