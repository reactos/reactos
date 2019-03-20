
spec2def(ks.sys ks.spec ADD_IMPORTLIB)

add_definitions(
    -DBUILDING_KS
    -D_COMDDK_)

list(APPEND SOURCE
    api.c
    allocators.c
    bag.c
    device.c
    deviceinterface.c
    driver.c
    clocks.c
    connectivity.c
    event.c
    filter.c
    filterfactory.c
    image.c
    irp.c
    methods.c
    misc.c
    pin.c
    property.c
    topology.c
    unknown.c
    worker.c
    kcom.c
    swenum.c
    precomp.h)

if(MSVC)
    add_asm_files(ks_asm msvc.S)
endif()

add_library(ks MODULE
    ${SOURCE}
    ${ks_asm}
    ks.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ks.def)

set_module_type(ks kernelmodedriver ENTRYPOINT 0)

target_link_libraries(ks ${PSEH_LIB})
add_pch(ks precomp.h SOURCE)
add_importlibs(ks ntoskrnl hal)
add_dependencies(ks psdk)
add_cd_file(TARGET ks DESTINATION reactos/system32/drivers FOR all)
