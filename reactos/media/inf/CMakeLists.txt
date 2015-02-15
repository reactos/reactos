
list(APPEND INF_FILES
    acpi.inf
    audio.inf
    battery.inf
    bda.inf
    bth.inf
    c_image.inf
    cdrom.inf
    cpu.inf
    defltws.inf
    disk.inf
    display.inf
    fdc.inf
    flpydisk.inf
    font.inf
    hal.inf
    hdc.inf
    input.inf
    intl.inf
    keyboard.inf
    ks.inf
    kscaptur.inf
    layout.inf
    legcydrv.inf
    machine.inf
    monitor.inf
    msmouse.inf
    NET_NIC.inf
    netamd.inf
    netisa.inf
    netrtl.inf
    netrtpnt.inf
    nettcpip.inf
    ports.inf
    scsi.inf
    shortcuts.inf
    unknown.inf
    usb.inf
    usbport.inf
    usbstor.inf
    wdmaudio.inf
    xboxdisp.inf)

foreach(item ${INF_FILES})
    add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/${item}" DESTINATION reactos/inf FOR all)
endforeach(item)

add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/syssetup.inf DESTINATION reactos/inf FOR all)
