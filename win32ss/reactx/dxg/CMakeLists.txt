
spec2def(dxg.sys dxg.spec)

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

include_directories(
    ${REACTOS_SOURCE_DIR}/lib/pseh/include
    ${REACTOS_SOURCE_DIR}/win32ss/include)

add_library(dxg MODULE
    ${SOURCE}
    dxg.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dxg.def)

set_module_type(dxg kernelmodedriver)
target_link_libraries(dxg ${PSEH_LIB})
add_importlibs(dxg dxgthk ntoskrnl msvcrt kernel32 ntdll)
add_pch(dxg dxg_int.h SOURCE)
add_cd_file(TARGET dxg DESTINATION reactos/system32/drivers FOR all)
