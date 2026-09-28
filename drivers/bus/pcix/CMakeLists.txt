
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers)

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
    pci/ecam.c
    pci/express.c
    pci/id.c
    pci/msi.c
    pci/ppbridge.c
    pci/rbar.c
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

add_library(pci MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    pci.rc)
add_pch(pci pci.h "${PCH_SKIP_SOURCE}")

set_module_type(pci kernelmodedriver)
target_link_libraries(pci arbiter)
if(MSVC)
    target_link_options(pci PRIVATE /SECTION:.rsrc,!D)
else()
    add_custom_command(
        TARGET pci POST_BUILD
        COMMAND native-pefixup --section:.rsrc,!D $<TARGET_FILE:pci>
        VERBATIM)
endif()
add_importlibs(pci ntoskrnl hal)
add_dependencies(pci pciclass)
add_cd_file(TARGET pci DESTINATION reactos/system32/drivers NO_CAB FOR all)
