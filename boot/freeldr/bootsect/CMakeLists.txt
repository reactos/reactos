
CreateBootSectorTarget(dosmbr ${CMAKE_CURRENT_SOURCE_DIR}/dosmbr.asm ${CMAKE_CURRENT_BINARY_DIR}/dosmbr.bin)
CreateBootSectorTarget(ext2 ${CMAKE_CURRENT_SOURCE_DIR}/ext2.asm ${CMAKE_CURRENT_BINARY_DIR}/ext2.bin)
CreateBootSectorTarget(fat32 ${CMAKE_CURRENT_SOURCE_DIR}/fat32.asm ${CMAKE_CURRENT_BINARY_DIR}/fat32.bin)
CreateBootSectorTarget(fat ${CMAKE_CURRENT_SOURCE_DIR}/fat.asm ${CMAKE_CURRENT_BINARY_DIR}/fat.bin)
CreateBootSectorTarget(isoboot ${CMAKE_CURRENT_SOURCE_DIR}/isoboot.asm ${CMAKE_CURRENT_BINARY_DIR}/isoboot.bin)
CreateBootSectorTarget(isobtrt ${CMAKE_CURRENT_SOURCE_DIR}/isobtrt.asm ${CMAKE_CURRENT_BINARY_DIR}/isobtrt.bin)