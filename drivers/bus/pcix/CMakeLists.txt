
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/arbiter
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers)

list(APPEND SOURCE
    arb/ar_busno.c
    arb/ar_memio.c
    arb/arb_comn.c
    arb/tr_irq.c
    intrface/agpintrf.c
    intrface/busintrf.c
    intrface/cardbus.c
    intrface/devhere.c
    intrface/ideintrf.c
    intrface/intrface.c
    intrface/lddintrf.c
    intrface/locintrf.c
    intrface/pmeintf.c
    intrface/routintf.c
    pci/busno.c
    pci/config.c
    pci/devhere.c
    pci/id.c
    pci/ppbridge.c
    pci/romimage.c
    pci/state.c
    debug.c
    device.c
    dispatch.c
    enum.c
    fdo.c
    hookhal.c
    init.c
    pcivrify.c
    pdo.c
    power.c
    usage.c
    utils.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_library(pcix MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    pci.rc)

set_module_type(pcix kernelmodedriver)
target_link_libraries(pcix arbiter)
if(MSVC)
    target_link_options(pcix PRIVATE /SECTION:.rsrc,!D)
else()
    add_custom_command(
        TARGET pcix POST_BUILD
        COMMAND native-pefixup --section:.rsrc,!D $<TARGET_FILE:pcix>
        VERBATIM)
endif()
add_importlibs(pcix ntoskrnl hal)
add_pch(pcix pci.h "${PCH_SKIP_SOURCE}")
add_dependencies(pcix pciclass)
add_cd_file(TARGET pcix DESTINATION reactos/system32/drivers NO_CAB FOR all)
