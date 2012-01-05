list(APPEND INF_FILES
    acpi.inf
    audio.inf
    battery.inf
    bda.inf
    cdrom.inf
    cpu.inf
    display.inf
    fdc.inf
    font.inf
    hdc.inf
    input.inf
    intl.inf
    keyboard.inf
    ks.inf
    kscaptur.inf
    layout.inf
    machine.inf
    msmouse.inf
    NET_NIC.inf
    netamd.inf
    netisa.inf
    netrtpnt.inf
    nettcpip.inf
    ports.inf
    scsi.inf
    usb.inf
    usbport.inf
    usbstor.inf
    xboxdisp.inf
)

foreach(item ${INF_FILES})
    add_cab("${CMAKE_CURRENT_SOURCE_DIR}/${item}" 6)
endforeach(item)

add_cab(${CMAKE_CURRENT_SOURCE_DIR}/syssetup.inf 6)