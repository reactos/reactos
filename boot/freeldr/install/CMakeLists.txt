
#####################################
# Generate the FAT bootsector source files
#
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/fat.c ${CMAKE_CURRENT_BINARY_DIR}/fat.h
    COMMAND native-bin2c ${CMAKE_CURRENT_BINARY_DIR}/../bootsect/fat.bin ${CMAKE_CURRENT_BINARY_DIR}/fat.c ${CMAKE_CURRENT_BINARY_DIR}/fat.h fat_data
    DEPENDS native-bin2c fat)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/fat32.c ${CMAKE_CURRENT_BINARY_DIR}/fat32.h
    COMMAND native-bin2c ${CMAKE_CURRENT_BINARY_DIR}/../bootsect/fat32.bin ${CMAKE_CURRENT_BINARY_DIR}/fat32.c ${CMAKE_CURRENT_BINARY_DIR}/fat32.h fat32_data
    DEPENDS native-bin2c fat32)
#####################################

add_executable(installfreeldr EXCLUDE_FROM_ALL
    install.c
    volume.c
    ${CMAKE_CURRENT_BINARY_DIR}/fat.c
    ${CMAKE_CURRENT_BINARY_DIR}/fat32.c)

set_module_type(installfreeldr win32cui)
add_importlibs(installfreeldr msvcrt kernel32)
