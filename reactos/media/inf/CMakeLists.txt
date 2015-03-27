
list(APPEND INF_FILES
    acpi.inf
    audio.inf
    battery.inf
    bda.inf
    bth.inf
    c_image.inf
    cdrom.inf
    cpu.inf
    disk.inf
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
    wmiacpi.inf
    xboxdisp.inf)

list(APPEND UTF8_INF_FILES
    defltws.inf
    font.inf
    intl.inf
    layout.inf)

foreach(item ${INF_FILES})
    set(_converted_item ${CMAKE_CURRENT_BINARY_DIR}/${item})
    set(_source_item ${CMAKE_CURRENT_SOURCE_DIR}/${item})
    add_custom_command(OUTPUT "${_converted_item}"
                       COMMAND native-utf16le "${_source_item}" "${_converted_item}"
                       DEPENDS native-utf16le "${_source_item}")
    add_cd_file(TARGET converted_inf_files FILE ${_converted_item} DESTINATION reactos/inf FOR all)
    list(APPEND _converted_inf_files ${_converted_item})
endforeach(item)

add_custom_target(converted_inf_files DEPENDS ${_converted_inf_files})

foreach(item ${UTF8_INF_FILES})
    add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/${item}" DESTINATION reactos/inf FOR all)
endforeach(item)

add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/syssetup.inf DESTINATION reactos/inf FOR all)
