
CreateBootSectorTarget(dosmbr ${CMAKE_CURRENT_SOURCE_DIR}/freeldr/bootsect/dosmbr.asm ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/dosmbr.bin)
CreateBootSectorTarget(ext2 ${CMAKE_CURRENT_SOURCE_DIR}/freeldr/bootsect/ext2.asm ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/ext2.bin)
CreateBootSectorTarget(fat32 ${CMAKE_CURRENT_SOURCE_DIR}/freeldr/bootsect/fat32.asm ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/fat32.bin)
CreateBootSectorTarget(fat ${CMAKE_CURRENT_SOURCE_DIR}/freeldr/bootsect/fat.asm ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/fat.bin)
CreateBootSectorTarget(isoboot ${CMAKE_CURRENT_SOURCE_DIR}/freeldr/bootsect/isoboot.asm ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isoboot.bin)
CreateBootSectorTarget(isobtrt ${CMAKE_CURRENT_SOURCE_DIR}/freeldr/bootsect/isobtrt.asm ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isobtrt.bin)
