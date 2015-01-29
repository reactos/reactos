
if(ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64")

    CreateBootSectorTarget(dosmbr ${CMAKE_CURRENT_SOURCE_DIR}/dosmbr.S ${CMAKE_CURRENT_BINARY_DIR}/dosmbr.bin 7c00)
    CreateBootSectorTarget(ext2 ${CMAKE_CURRENT_SOURCE_DIR}/ext2.S ${CMAKE_CURRENT_BINARY_DIR}/ext2.bin 0)
    CreateBootSectorTarget(fat ${CMAKE_CURRENT_SOURCE_DIR}/fatx.S ${CMAKE_CURRENT_BINARY_DIR}/fat.bin 7c00)
    CreateBootSectorTarget(fat32 ${CMAKE_CURRENT_SOURCE_DIR}/fat32.S ${CMAKE_CURRENT_BINARY_DIR}/fat32.bin 7c00)
    CreateBootSectorTarget(isoboot ${CMAKE_CURRENT_SOURCE_DIR}/isoboot.S ${CMAKE_CURRENT_BINARY_DIR}/isoboot.bin 7000)
    CreateBootSectorTarget(isobtrt ${CMAKE_CURRENT_SOURCE_DIR}/isobtrt.S ${CMAKE_CURRENT_BINARY_DIR}/isobtrt.bin 7000)

    add_cd_file(TARGET dosmbr DESTINATION loader NO_CAB FILE ${CMAKE_CURRENT_BINARY_DIR}/dosmbr.bin FOR bootcd regtest)
    add_cd_file(TARGET ext2 DESTINATION loader NO_CAB FILE ${CMAKE_CURRENT_BINARY_DIR}/ext2.bin FOR bootcd regtest)
    add_cd_file(TARGET fat DESTINATION loader NO_CAB FILE ${CMAKE_CURRENT_BINARY_DIR}/fat.bin FOR bootcd regtest)
    add_cd_file(TARGET fat32 DESTINATION loader NO_CAB FILE ${CMAKE_CURRENT_BINARY_DIR}/fat32.bin FOR bootcd regtest)
    add_cd_file(TARGET isoboot DESTINATION loader NO_CAB NOT_IN_HYBRIDCD FILE ${CMAKE_CURRENT_BINARY_DIR}/isoboot.bin FOR all hybridcd)
    add_cd_file(TARGET isobtrt DESTINATION loader NO_CAB NOT_IN_HYBRIDCD FILE ${CMAKE_CURRENT_BINARY_DIR}/isobtrt.bin FOR bootcd regtest)

endif()
