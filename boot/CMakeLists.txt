## efisys.bin

# EFI platform ID, used in environ/CMakelists.txt for bootmgfw filename naming also.
if(ARCH STREQUAL "amd64")
    set(EFI_PLATFORM_ID "x64")
elseif(ARCH STREQUAL "i386")
    set(EFI_PLATFORM_ID "ia32")
elseif(ARCH STREQUAL "ia64")
    set(EFI_PLATFORM_ID "ia64")
elseif(ARCH STREQUAL "arm")
    set(EFI_PLATFORM_ID "arm")
elseif(ARCH STREQUAL "aarch64")
    set(EFI_PLATFORM_ID "aa64")
else()
    message(FATAL_ERROR "Unknown ARCH '" ${ARCH} "', cannot generate a valid UEFI boot filename.")
endif()

add_custom_target(efisys
    COMMAND native-fatten ${CMAKE_CURRENT_BINARY_DIR}/efisys.bin -format 2880 EFIBOOT -boot ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/fat.bin -mkdir EFI -mkdir EFI/BOOT -add $<TARGET_FILE:bootmgfw> EFI/BOOT/boot${EFI_PLATFORM_ID}.efi
    DEPENDS native-fatten fat bootmgfw
    VERBATIM)


# Create an 'empty' directory (guaranteed to be empty) to be able to add
# arbitrary empty directories to the ISO image using mkisofs.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/empty)

# Retrieve the full paths to the generated files of the 'isombr', 'isoboot', 'isobtrt' and 'efisys' targets
set(_isombr_file  ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isombr.bin)  # get_target_property(_isombr_file  isombr  LOCATION)
set(_isoboot_file ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isoboot.bin) # get_target_property(_isoboot_file isoboot LOCATION)
set(_isobtrt_file ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isobtrt.bin) # get_target_property(_isobtrt_file isobtrt LOCATION)
set(_efisys_file  ${CMAKE_CURRENT_BINARY_DIR}/efisys.bin) # get_target_property(_efisys_file  efisys  LOCATION)

# Create a mkisofs sort file to specify an explicit ordering for the boot files
# to place them at the beginning of the image (makes ISO image analysis easier).
# See mkisofs/schilytools/mkisofs/README.sort for more details.
# As the default file sort weight is '0', give the boot files sort weights >= 1.
# Note that it is sad that '-sort' does not work using grafted points, and as a
# result we need in particular to use the boot catalog file "path" mkisofs that
# mkisofs expects, that is, the boot catalog file name is appended to the first
# host-system path listed in the file list, whatever it is, and that does not
# work well if the first item is a graft point (and especially if it's a file
# and not a directory). To fix that, the trick is to use as the first file item
# the empty directory created earlier. This ensures that:
# - the boot catalog file path is meaningful;
# - since its contents are included by mkisofs in the root of the ISO image,
#   using the empty directory ensures that no extra unwanted files are added.
#
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bootfiles.sort "\
${CMAKE_CURRENT_BINARY_DIR}/empty/boot.catalog 4
${_isoboot_file} 3
${_isobtrt_file} 2
${_efisys_file} 1
")

# ISO image identificator names
set(ISO_MANUFACTURER "ReactOS Foundation")  # For both the publisher and the preparer
set(ISO_VOLNAME      "ReactOS")             # For both the Volume ID and the Volume set ID

## BootCD
# Create the file list
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bootcd.lst "")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bootcd.lst "${CMAKE_CURRENT_BINARY_DIR}/empty\n")

add_custom_target(bootcd
    COMMAND native-mkisofs -quiet -o ${REACTOS_BINARY_DIR}/bootcd.iso -iso-level 4
        -publisher ${ISO_MANUFACTURER} -preparer ${ISO_MANUFACTURER} -volid ${ISO_VOLNAME} -volset ${ISO_VOLNAME}
        -eltorito-boot loader/isoboot.bin -no-emul-boot -boot-load-size 4 -eltorito-alt-boot -eltorito-platform efi -eltorito-boot loader/efisys.bin -no-emul-boot -hide boot.catalog
        -sort ${CMAKE_CURRENT_BINARY_DIR}/bootfiles.sort
        -duplicates-once -no-cache-inodes -graft-points -path-list ${CMAKE_CURRENT_BINARY_DIR}/bootcd.lst
    COMMAND native-isohybrid -b ${_isombr_file} ${REACTOS_BINARY_DIR}/bootcd.iso
    DEPENDS isombr native-isohybrid native-mkisofs livecd
    VERBATIM)

## BootCDRegTest
# Create the file list
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bootcdregtest.lst "")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bootcdregtest.lst "${CMAKE_CURRENT_BINARY_DIR}/empty\n")

add_custom_target(bootcdregtest
    COMMAND native-mkisofs -quiet -o ${REACTOS_BINARY_DIR}/bootcdregtest.iso -iso-level 4
        -publisher ${ISO_MANUFACTURER} -preparer ${ISO_MANUFACTURER} -volid ${ISO_VOLNAME} -volset ${ISO_VOLNAME}
        -eltorito-boot loader/isobtrt.bin -no-emul-boot -boot-load-size 4 -eltorito-alt-boot -eltorito-platform efi -eltorito-boot loader/efisys.bin -no-emul-boot -hide boot.catalog
        -sort ${CMAKE_CURRENT_BINARY_DIR}/bootfiles.sort
        -duplicates-once -no-cache-inodes -graft-points -path-list ${CMAKE_CURRENT_BINARY_DIR}/bootcdregtest.lst
    COMMAND native-isohybrid -b ${_isombr_file} ${REACTOS_BINARY_DIR}/bootcdregtest.iso
    DEPENDS isombr native-isohybrid native-mkisofs
    VERBATIM)

## LiveImage -- Constitutes a small RAMDISK ISO, and is also merged with the regular BootCD
# Create the file list
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "${CMAKE_CURRENT_BINARY_DIR}/empty\n")

# Create the empty Desktop, Favorites, and Start Menu folders. And many more.
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/All Users/Application Data=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/All Users/Documents/My Music=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/All Users/Documents/My Pictures=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/All Users/Documents/My Videos=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/All Users/Favorites=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/All Users/My Documents=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/All Users/Start Menu/Programs/StartUp=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/All Users/Templates=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Application Data=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Cookies=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Desktop=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Favorites=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Local Settings/Application Data=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Local Settings/History=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Local Settings/Temporary Internet Files=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/My Music=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/My Pictures=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/My Videos=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/NetHood=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/PrintHood=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Recent=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/SendTo=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Start Menu/Programs=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Start Menu/Programs/Administrative Tools=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Start Menu/Programs/StartUp=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Templates=${CMAKE_CURRENT_BINARY_DIR}/empty\n")

add_custom_target(livecd
    COMMAND native-mkisofs -quiet -o ${REACTOS_BINARY_DIR}/liveimage.iso -iso-level 4
        -publisher ${ISO_MANUFACTURER} -preparer ${ISO_MANUFACTURER} -volid ${ISO_VOLNAME} -volset ${ISO_VOLNAME}
        -duplicates-once -no-cache-inodes -graft-points -path-list ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst
    DEPENDS native-mkisofs
    VERBATIM)


add_cd_file(TARGET efisys FILE ${CMAKE_CURRENT_BINARY_DIR}/efisys.bin DESTINATION loader NO_CAB FOR bootcd regtest)

add_subdirectory(freeldr)
add_subdirectory(bootdata)
add_subdirectory(environ)
