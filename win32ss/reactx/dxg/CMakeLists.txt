
include_directories(
    ${REACTOS_SOURCE_DIR}/win32ss/include)

list(APPEND SOURCE
    main.c
    ddhmg.c
    d3d.c
    dd.c
    ddraw.c
    ddsurf.c
    eng.c
    historic.c
    dxg_int.h)

add_library(dxg MODULE
    ${SOURCE}
    dxg.rc)
spec2def(dxg dxg.sys dxg.spec)

set_module_type(dxg kernelmodedriver)
add_importlibs(dxg dxgthk ntoskrnl)
add_pch(dxg dxg_int.h SOURCE)
add_cd_file(TARGET dxg DESTINATION reactos/system32/drivers FOR all)
