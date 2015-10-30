
add_subdirectory(fonts)
add_subdirectory(inf)
add_subdirectory(nls)
add_subdirectory(themes)
add_subdirectory(vgafonts)

add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/drivers/etc/hosts DESTINATION reactos/system32/drivers/etc FOR all)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/drivers/etc/KDBinit DESTINATION reactos/system32/drivers/etc FOR all)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/drivers/etc/networks DESTINATION reactos/system32/drivers/etc FOR all)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/drivers/etc/protocol DESTINATION reactos/system32/drivers/etc FOR all)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/drivers/etc/services DESTINATION reactos/system32/drivers/etc FOR all)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/sounds/ReactOS_LogOn.wav DESTINATION reactos/media FOR all)
