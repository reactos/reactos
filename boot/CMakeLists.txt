
add_subdirectory(freeldr)

add_minicd(${CMAKE_CURRENT_SOURCE_DIR}/bootdata/txtsetup.sif reactos txtsetup.sif)
add_minicd(${CMAKE_CURRENT_SOURCE_DIR}/bootdata/bootcd.ini "" freeldr.ini)
add_minicd(${CMAKE_CURRENT_SOURCE_DIR}/bootdata/hivecls_${ARCH}.inf reactos hivecls.inf)
add_minicd(${CMAKE_CURRENT_SOURCE_DIR}/bootdata/hivedef_${ARCH}.inf reactos hivedef.inf)
add_minicd(${CMAKE_CURRENT_SOURCE_DIR}/bootdata/hivesft_${ARCH}.inf reactos hivesft.inf)
add_minicd(${CMAKE_CURRENT_SOURCE_DIR}/bootdata/hivesys_${ARCH}.inf reactos hivesys.inf)
add_minicd(${CMAKE_CURRENT_SOURCE_DIR}/bootdata/autorun.inf "" autorun.inf)
add_minicd(${CMAKE_CURRENT_SOURCE_DIR}/bootdata/icon.ico "" icon.ico)
add_minicd(${CMAKE_CURRENT_SOURCE_DIR}/bootdata/readme.txt "" readme.txt)
# Livecd files
list(APPEND LIVECD_HIVES
    ${CMAKE_CURRENT_SOURCE_DIR}/bootdata/livecd.inf
    ${CMAKE_CURRENT_SOURCE_DIR}/bootdata/hiveinst_${ARCH}.inf)

add_custom_command(
    OUTPUT ${LIVECD_DIR}/reactos/system32/config/sam
    COMMAND native-mkhive ${CMAKE_CURRENT_SOURCE_DIR}/bootdata ${LIVECD_DIR}/reactos/system32/config ${ARCH} ${LIVECD_HIVES}
    DEPENDS native-mkhive)

add_custom_target(livecd_hives DEPENDS ${LIVECD_DIR}/reactos/system32/config/sam)

add_livecd(${REACTOS_SOURCE_DIR}/boot/bootdata/livecd.ini "" freeldr.ini)

#regtest
add_cab(${REACTOS_SOURCE_DIR}/boot/bootdata/bootcdregtest/regtest.cmd 7)
