
list(APPEND INF_FILES
    audio.inf
    battery.inf
    bda.inf
    bth.inf
    c_image.inf
    display.inf
    fdc.inf
    flpydisk.inf
    hal.inf
    hdc.inf
    input.inf
    keyboard.inf
    ks.inf
    kscaptur.inf
    legcydrv.inf
    machine.inf
    monitor.inf
    msmouse.inf
    NET_NIC.inf
    nettcpip.inf
    ports.inf
    scsi.inf
    shortcuts.inf
    unknown.inf
    usb.inf
    usbport.inf
    wdmaudio.inf
    wmiacpi.inf)

list(APPEND UTF8_INF_FILES
    defltws.inf
    font.inf
    intl.inf
    layout.inf)

# module name is used for a custom target creation, so we can put any unique name here
add_driver_inf(media_inf ${INF_FILES})

foreach(item ${UTF8_INF_FILES})
    add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/${item}" DESTINATION reactos/inf FOR all)
endforeach(item)

add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/syssetup.inf DESTINATION reactos/inf FOR all)
