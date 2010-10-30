
list(APPEND SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/437-8x8.bin
    ${CMAKE_CURRENT_SOURCE_DIR}/737-8x8.bin
    ${CMAKE_CURRENT_SOURCE_DIR}/775-8x8.bin
    ${CMAKE_CURRENT_SOURCE_DIR}/850-8x8.bin
    ${CMAKE_CURRENT_SOURCE_DIR}/852-8x8.bin
    ${CMAKE_CURRENT_SOURCE_DIR}/865-8x8.bin 
    ${CMAKE_CURRENT_SOURCE_DIR}/866-8x8.bin
    ${CMAKE_CURRENT_SOURCE_DIR}/932-8x8.bin
)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vgafonts.cab
                   COMMAND native-cabman -M raw -S ${CMAKE_CURRENT_BINARY_DIR}/vgafonts.cab ${SOURCE}
                   DEPENDS ${SOURCE})
                   
add_custom_target(vgafonts DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/vgafonts.cab)

add_minicd(${CMAKE_CURRENT_BINARY_DIR}/vgafonts.cab reactos vgafonts.cab)