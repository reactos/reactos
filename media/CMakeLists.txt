add_subdirectory(fonts)
add_subdirectory(inf)
add_subdirectory(nls)
add_subdirectory(vgafonts)

add_minicd(${CMAKE_CURRENT_SOURCE_DIR}/drivers/etc/hosts reactos hosts)
add_minicd(${CMAKE_CURRENT_SOURCE_DIR}/drivers/etc/KDBinit reactos KDBinit)
add_minicd(${CMAKE_CURRENT_SOURCE_DIR}/drivers/etc/services reactos services)

add_livecd(${CMAKE_CURRENT_SOURCE_DIR}/drivers/etc/hosts reactos/system32/drivers/etc hosts)
add_livecd(${CMAKE_CURRENT_SOURCE_DIR}/drivers/etc/KDBinit reactos/system32/drivers/etc KDBinit)
add_livecd(${CMAKE_CURRENT_SOURCE_DIR}/drivers/etc/services reactos/system32/drivers/etc services)