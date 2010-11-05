add_subdirectory(fonts)
add_subdirectory(inf)
add_subdirectory(nls)
add_subdirectory(vgafonts)

add_minicd(${CMAKE_CURRENT_SOURCE_DIR}/drivers/etc/hosts reactos hosts)
add_minicd(${CMAKE_CURRENT_SOURCE_DIR}/drivers/etc/KDBinit reactos KDBinit)
add_minicd(${CMAKE_CURRENT_SOURCE_DIR}/drivers/etc/services reactos services)

add_cab(${CMAKE_CURRENT_SOURCE_DIR}/drivers/etc/hosts 5)
add_cab(${CMAKE_CURRENT_SOURCE_DIR}/drivers/etc/KDBinit 5)
add_cab(${CMAKE_CURRENT_SOURCE_DIR}/drivers/etc/services 5)
add_cab(${CMAKE_CURRENT_SOURCE_DIR}/sounds/ReactOS_LogOn.wav 8)