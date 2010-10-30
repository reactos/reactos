
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
    guid.c
    hookhal.c
    init.c
    pcivrify.c
    pdo.c
    power.c
    usage.c
    utils.c
    pci.rc)

add_library(pcix SHARED ${CMAKE_CURRENT_BINARY_DIR}/pcix_pci.h.gch ${SOURCE})

set_target_properties(pcix PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native -Wl,--exclude-all-symbols" SUFFIX ".sys")

add_importlibs(pcix ntoskrnl hal)

add_pch(pcix ${CMAKE_CURRENT_SOURCE_DIR}/pci.h ${SOURCE})
add_dependencies(pcix psdk pciclass buildno_header)
