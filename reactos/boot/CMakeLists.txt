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
    DEPENDS native-fatten fat bootmgfw bcd_hive
    VERBATIM)

if(USE_MKISOFS)
# Create an 'empty' directory (guaranteed to be empty) to be able to add
# arbitrary empty directories to the ISO image using mkisofs.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/empty)
endif()

# Retrieve the full paths to the generated files of the 'isoboot', 'isobtrt' and 'efisys' targets
set(_isoboot_file ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isoboot.bin) # get_target_property(_isoboot_file isoboot LOCATION)
set(_isobtrt_file ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isobtrt.bin) # get_target_property(_isobtrt_file isobtrt LOCATION)
set(_efisys_file  ${CMAKE_CURRENT_BINARY_DIR}/efisys.bin) # get_target_property(_efisys_file  efisys  LOCATION)

if(USE_MKISOFS)
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
endif()

# ISO image identificator names
set(ISO_MANUFACTURER "ReactOS Foundation")  # For both the publisher and the preparer
set(ISO_VOLNAME      "ReactOS")             # For both the Volume ID and the Volume set ID

## BootCD
# Create the file list
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bootcd.lst "")

if(USE_MKISOFS)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bootcd.lst "${CMAKE_CURRENT_BINARY_DIR}/empty\n")

    add_custom_target(bootcd
        COMMAND native-mkisofs -o ${REACTOS_BINARY_DIR}/bootcd.iso -iso-level 4
            -publisher ${ISO_MANUFACTURER} -preparer ${ISO_MANUFACTURER} -volid ${ISO_VOLNAME} -volset ${ISO_VOLNAME}
            -eltorito-boot loader/isoboot.bin -no-emul-boot -boot-load-size 4 -eltorito-alt-boot -eltorito-platform efi -eltorito-boot loader/efisys.bin -no-emul-boot -hide boot.catalog
            -sort ${CMAKE_CURRENT_BINARY_DIR}/bootfiles.sort
            -no-cache-inodes -graft-points -path-list ${CMAKE_CURRENT_BINARY_DIR}/bootcd.lst
        DEPENDS native-mkisofs
        VERBATIM)
else()
    add_custom_target(bootcd
        COMMAND native-cdmake -j -m -bootdata:2\#p0,e,b${_isoboot_file}\#pEF,e,b${_efisys_file} @${CMAKE_CURRENT_BINARY_DIR}/bootcd.lst REACTOS ${REACTOS_BINARY_DIR}/bootcd.iso
        DEPENDS native-cdmake efisys
        VERBATIM)
endif()

## BootCDRegTest
# Create the file list
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bootcdregtest.lst "")

if(USE_MKISOFS)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bootcdregtest.lst "${CMAKE_CURRENT_BINARY_DIR}/empty\n")

    add_custom_target(bootcdregtest
        COMMAND native-mkisofs -o ${REACTOS_BINARY_DIR}/bootcdregtest.iso -iso-level 4
            -publisher ${ISO_MANUFACTURER} -preparer ${ISO_MANUFACTURER} -volid ${ISO_VOLNAME} -volset ${ISO_VOLNAME}
            -eltorito-boot loader/isobtrt.bin -no-emul-boot -boot-load-size 4 -eltorito-alt-boot -eltorito-platform efi -eltorito-boot loader/efisys.bin -no-emul-boot -hide boot.catalog
            -sort ${CMAKE_CURRENT_BINARY_DIR}/bootfiles.sort
            -no-cache-inodes -graft-points -path-list ${CMAKE_CURRENT_BINARY_DIR}/bootcdregtest.lst
        DEPENDS native-mkisofs
        VERBATIM)
else()
    add_custom_target(bootcdregtest
        COMMAND native-cdmake -j -m -bootdata:2\#p0,e,b${_isobtrt_file}\#pEF,e,b${_efisys_file} @${CMAKE_CURRENT_BINARY_DIR}/bootcdregtest.lst REACTOS ${REACTOS_BINARY_DIR}/bootcdregtest.iso
        DEPENDS native-cdmake efisys
        VERBATIM)
endif()

## LiveCD
# Create the file list
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "")

if(USE_MKISOFS)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "${CMAKE_CURRENT_BINARY_DIR}/empty\n")

    # Create the empty Desktop, Favorites, and Start Menu folders
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Desktop=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Favorites=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Start Menu/Programs=${CMAKE_CURRENT_BINARY_DIR}/empty\n")

    add_custom_target(livecd
        COMMAND native-mkisofs -o ${REACTOS_BINARY_DIR}/livecd.iso -iso-level 4
            -publisher ${ISO_MANUFACTURER} -preparer ${ISO_MANUFACTURER} -volid ${ISO_VOLNAME} -volset ${ISO_VOLNAME}
            -eltorito-boot loader/isoboot.bin -no-emul-boot -boot-load-size 4 -eltorito-alt-boot -eltorito-platform efi -eltorito-boot loader/efisys.bin -no-emul-boot -hide boot.catalog
            -sort ${CMAKE_CURRENT_BINARY_DIR}/bootfiles.sort
            -no-cache-inodes -graft-points -path-list ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst
        DEPENDS native-mkisofs
        VERBATIM)
else()
    # Create the empty Desktop, Favorites, and Start Menu folders
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Desktop\n")
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Favorites\n")
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Start Menu/Programs\n")

    add_custom_target(livecd
        COMMAND native-cdmake -j -m -bootdata:2\#p0,e,b${_isoboot_file}\#pEF,e,b${_efisys_file} @${CMAKE_CURRENT_BINARY_DIR}/livecd.lst REACTOS ${REACTOS_BINARY_DIR}/livecd.iso
        DEPENDS native-cdmake efisys
        VERBATIM)
endif()

## HybridCD
# Create the file list
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "")

if(USE_MKISOFS)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "${CMAKE_CURRENT_BINARY_DIR}/empty\n")

    # Create the empty Desktop, Favorites, and Start Menu folders
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "livecd/Profiles/Default User/Desktop=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "livecd/Profiles/Default User/Favorites=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "livecd/Profiles/Default User/Start Menu/Programs=${CMAKE_CURRENT_BINARY_DIR}/empty\n")

    add_custom_target(hybridcd
        COMMAND native-mkisofs -o ${REACTOS_BINARY_DIR}/hybridcd.iso -iso-level 4
            -publisher ${ISO_MANUFACTURER} -preparer ${ISO_MANUFACTURER} -volid ${ISO_VOLNAME} -volset ${ISO_VOLNAME}
            -eltorito-boot loader/isoboot.bin -no-emul-boot -boot-load-size 4 -eltorito-alt-boot -eltorito-platform efi -eltorito-boot loader/efisys.bin -no-emul-boot -hide boot.catalog
            -sort ${CMAKE_CURRENT_BINARY_DIR}/bootfiles.sort
            -no-cache-inodes -graft-points -path-list ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst
        DEPENDS native-mkisofs bootcd livecd
        VERBATIM)
else()
    # Create the empty Desktop, Favorites, and Start Menu folders
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "livecd/Profiles/Default User/Desktop\n")
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "livecd/Profiles/Default User/Favorites\n")
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "livecd/Profiles/Default User/Start Menu/Programs\n")

    add_custom_target(hybridcd
        COMMAND native-cdmake -j -m -bootdata:2\#p0,e,b${_isoboot_file}\#pEF,e,b${_efisys_file} @${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst REACTOS ${REACTOS_BINARY_DIR}/hybridcd.iso
        DEPENDS native-cdmake efisys bootcd livecd
        VERBATIM)
endif()

if(USE_MKISOFS)
    add_cd_file(TARGET efisys FILE ${CMAKE_CURRENT_BINARY_DIR}/efisys.bin DESTINATION loader NO_CAB NOT_IN_HYBRIDCD FOR bootcd regtest livecd hybridcd)
endif()

add_subdirectory(freeldr)
add_subdirectory(bootdata)
add_subdirectory(environ)
