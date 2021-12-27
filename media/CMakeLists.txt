
add_subdirectory(drivers)
add_subdirectory(fonts)
add_subdirectory(inf)
add_subdirectory(nls)
add_subdirectory(sdb)
add_subdirectory(sounds)
add_subdirectory(themes)
add_subdirectory(vgafonts)

add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/systemcompatible.manifest DESTINATION reactos/winsxs/manifests FOR all)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/forwardcompatible.manifest DESTINATION reactos/winsxs/manifests FOR all)
