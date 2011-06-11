
if(ARCH MATCHES i386 OR ARCH MATCHES amd64)

CreateBootSectorTarget2(dosmbr ${CMAKE_CURRENT_SOURCE_DIR}/dosmbr.S ${CMAKE_CURRENT_BINARY_DIR}/dosmbr.bin 7c00)
#CreateBootSectorTarget2(ext2 ${CMAKE_CURRENT_SOURCE_DIR}/ext2.S ${CMAKE_CURRENT_BINARY_DIR}/ext2.bin 0)
CreateBootSectorTarget2(fat32 ${CMAKE_CURRENT_SOURCE_DIR}/fat32.S ${CMAKE_CURRENT_BINARY_DIR}/fat32.bin 7c00)
#CreateBootSectorTarget2(fat ${CMAKE_CURRENT_SOURCE_DIR}/fat.S ${CMAKE_CURRENT_BINARY_DIR}/fat.bin 0)
CreateBootSectorTarget2(isoboot ${CMAKE_CURRENT_SOURCE_DIR}/isoboot.S ${CMAKE_CURRENT_BINARY_DIR}/isoboot.bin 7000)
CreateBootSectorTarget2(isobtrt ${CMAKE_CURRENT_SOURCE_DIR}/isobtrt.S ${CMAKE_CURRENT_BINARY_DIR}/isobtrt.bin 7000)

if(NOT MSVC)
CreateBootSectorTarget(ext2 ${CMAKE_CURRENT_SOURCE_DIR}/ext2.asm ${CMAKE_CURRENT_BINARY_DIR}/ext2.bin 0)
CreateBootSectorTarget(fat ${CMAKE_CURRENT_SOURCE_DIR}/fat.asm ${CMAKE_CURRENT_BINARY_DIR}/fat.bin 0)
endif()

add_cd_file(TARGET dosmbr DESTINATION loader NO_CAB FILE ${CMAKE_CURRENT_BINARY_DIR}/dosmbr.bin FOR all)
add_cd_file(TARGET ext2 DESTINATION loader NO_CAB FILE ${CMAKE_CURRENT_BINARY_DIR}/ext2.bin FOR all)
add_cd_file(TARGET fat32 DESTINATION loader NO_CAB FILE ${CMAKE_CURRENT_BINARY_DIR}/fat32.bin FOR all)
add_cd_file(TARGET fat DESTINATION loader NO_CAB FILE ${CMAKE_CURRENT_BINARY_DIR}/fat.bin FOR all)
add_cd_file(TARGET isoboot DESTINATION loader NO_CAB FILE ${CMAKE_CURRENT_BINARY_DIR}/isoboot.bin FOR all)
add_cd_file(TARGET isobtrt DESTINATION loader NO_CAB FILE ${CMAKE_CURRENT_BINARY_DIR}/isobtrt.bin FOR all)

endif()

