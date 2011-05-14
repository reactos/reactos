
add_subdirectory(packages)

add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/txtsetup.sif DESTINATION reactos NO_CAB NO_LIVECD)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/hivecls_${ARCH}.inf DESTINATION reactos NO_CAB NO_LIVECD NAME_ON_CD hivecls.inf)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/hivedef_${ARCH}.inf DESTINATION reactos NO_CAB NO_LIVECD NAME_ON_CD hivedef.inf)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/hivesft_${ARCH}.inf DESTINATION reactos NO_CAB NO_LIVECD NAME_ON_CD hivesft.inf)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/hivesys_${ARCH}.inf DESTINATION reactos NO_CAB NAME_ON_CD hivesys.inf)

# bootcd/livecd hives
list(APPEND CD_HIVES
    ${CMAKE_CURRENT_SOURCE_DIR}/livecd.inf
    ${CMAKE_CURRENT_SOURCE_DIR}/hiveinst_${ARCH}.inf)

add_custom_target(livecd_hives
    native-mkhive ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${ARCH} ${CD_HIVES}
    DEPENDS native-mkhive ${CD_HIVES})

#regtest
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/bootcdregtest/regtest.cmd DESTINATION reactos/bin)

#freeldr.ini
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/bootcd.ini DESTINATION root NO_CAB NAME_ON_CD freeldr.ini NO_LIVECD)
