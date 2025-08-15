## efisys.bin

# EFI platform ID, used in environ/CMakelists.txt for bootmgfw filename naming also.
if(ARCH STREQUAL "amd64")
    set(EFI_PLATFORM_ID "x64")
elseif(ARCH STREQUAL "i386")
    if(NOT (SARCH STREQUAL "pc98" OR SARCH STREQUAL "xbox"))
        set(EFI_PLATFORM_ID "ia32")
    endif()
elseif(ARCH STREQUAL "ia64")
    set(EFI_PLATFORM_ID "ia64")
elseif(ARCH STREQUAL "arm")
    set(EFI_PLATFORM_ID "arm")
elseif(ARCH STREQUAL "arm64")
    set(EFI_PLATFORM_ID "aa64")
else()
    message(FATAL_ERROR "Unknown ARCH '" ${ARCH} "', cannot generate a valid UEFI boot filename.")
endif()

if(DEFINED EFI_PLATFORM_ID)
    add_custom_target(efisys
        COMMAND native-fatten ${CMAKE_CURRENT_BINARY_DIR}/efisys.bin -format 2880 EFIBOOT
            -boot ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/fat.bin
            -mkdir EFI -mkdir EFI/BOOT -add $<TARGET_FILE:uefildr> EFI/BOOT/boot${EFI_PLATFORM_ID}.efi
        DEPENDS native-fatten fat uefildr
        VERBATIM)
endif()

# ISO image EFI boot parameters
set(ISO_EFI_BOOT_PARAMS)

# Create an 'empty' directory (guaranteed to be empty) to be able to add
# arbitrary empty directories to the ISO image using mkisofs.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/empty)

# Retrieve the full paths to the generated files of the 'isombr', 'isoboot', 'isobtrt' and 'efisys' targets
set(_isombr_file  ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isombr.bin)  # get_target_property(_isombr_file  isombr  LOCATION)
set(_isoboot_file ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isoboot.bin) # get_target_property(_isoboot_file isoboot LOCATION)
set(_isobtrt_file ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isobtrt.bin) # get_target_property(_isobtrt_file isobtrt LOCATION)
if(DEFINED EFI_PLATFORM_ID)
    set(_efisys_file  ${CMAKE_CURRENT_BINARY_DIR}/efisys.bin) # get_target_property(_efisys_file  efisys  LOCATION)
    list(APPEND ISO_EFI_BOOT_PARAMS -eltorito-alt-boot -eltorito-platform efi -eltorito-boot loader/efisys.bin -no-emul-boot)
endif()

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
set(ISO_SORT_FILE_DATA "\
${CMAKE_CURRENT_BINARY_DIR}/empty/boot.catalog 4
${_isoboot_file} 3
${_isobtrt_file} 2
")
if(DEFINED EFI_PLATFORM_ID)
    string(APPEND ISO_SORT_FILE_DATA "${_efisys_file} 1\n")
endif()
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bootfiles.sort ${ISO_SORT_FILE_DATA})

# ISO image identifier names
set(ISO_MANUFACTURER "ReactOS Project") # For both the publisher and the preparer
set(ISO_VOLNAME      "ReactOS")         # For both the Volume ID and the Volume set ID


# Create user profile directories in the LiveImage
function(add_allusers_profile_dirs _image_filelist _rootdir)
    file(APPEND ${_image_filelist} "${_rootdir}/All Users/Application Data=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/All Users/Documents/My Music=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/All Users/Documents/My Pictures=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/All Users/Documents/My Videos=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/All Users/Favorites=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/All Users/My Documents=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/All Users/Start Menu/Programs/StartUp=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/All Users/Templates=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
endfunction()
function(add_user_profile_dirs _image_filelist _rootdir _username)
    file(APPEND ${_image_filelist} "${_rootdir}/${_username}/Application Data=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/${_username}/Application Data/Microsoft/Internet Explorer/Quick Launch=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/${_username}/Cookies=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/${_username}/Desktop=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/${_username}/Favorites=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/${_username}/Local Settings/Application Data=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/${_username}/Local Settings/History=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/${_username}/Local Settings/Temporary Internet Files=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/${_username}/My Music=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/${_username}/My Pictures=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/${_username}/My Videos=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/${_username}/NetHood=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/${_username}/PrintHood=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/${_username}/Recent=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/${_username}/SendTo=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/${_username}/Start Menu/Programs=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/${_username}/Start Menu/Programs/Administrative Tools=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/${_username}/Start Menu/Programs/StartUp=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
    file(APPEND ${_image_filelist} "${_rootdir}/${_username}/Templates=${CMAKE_CURRENT_BINARY_DIR}/empty\n")
endfunction()


## BootCD
# Create the file list
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bootcd.cmake.lst "")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bootcd.cmake.lst "${CMAKE_CURRENT_BINARY_DIR}/empty\n")

add_custom_target(bootcd
    COMMAND native-mkisofs -quiet -o ${REACTOS_BINARY_DIR}/bootcd.iso -iso-level 4
        -publisher ${ISO_MANUFACTURER} -preparer ${ISO_MANUFACTURER} -volid ${ISO_VOLNAME} -volset ${ISO_VOLNAME}
        -eltorito-boot loader/isoboot.bin -no-emul-boot -boot-load-size 4 ${ISO_EFI_BOOT_PARAMS} -hide boot.catalog
        -sort ${CMAKE_CURRENT_BINARY_DIR}/bootfiles.sort
        -duplicates-once -no-cache-inodes -graft-points -path-list ${CMAKE_CURRENT_BINARY_DIR}/bootcd.$<CONFIG>.lst
    COMMAND native-isohybrid -b ${_isombr_file} -t 0x96 ${REACTOS_BINARY_DIR}/bootcd.iso
    DEPENDS isombr native-isohybrid native-mkisofs livecd
    VERBATIM)

## BootCDRegTest
# Create the file list
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bootcdregtest.cmake.lst "")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bootcdregtest.cmake.lst "${CMAKE_CURRENT_BINARY_DIR}/empty\n")

add_custom_target(bootcdregtest
    COMMAND native-mkisofs -quiet -o ${REACTOS_BINARY_DIR}/bootcdregtest.iso -iso-level 4
        -publisher ${ISO_MANUFACTURER} -preparer ${ISO_MANUFACTURER} -volid ${ISO_VOLNAME} -volset ${ISO_VOLNAME}
        -eltorito-boot loader/isobtrt.bin -no-emul-boot -boot-load-size 4 ${ISO_EFI_BOOT_PARAMS} -hide boot.catalog
        -sort ${CMAKE_CURRENT_BINARY_DIR}/bootfiles.sort
        -duplicates-once -no-cache-inodes -graft-points -path-list ${CMAKE_CURRENT_BINARY_DIR}/bootcdregtest.$<CONFIG>.lst
    COMMAND native-isohybrid -b ${_isombr_file} -t 0x96 ${REACTOS_BINARY_DIR}/bootcdregtest.iso
    DEPENDS isombr native-isohybrid native-mkisofs
    VERBATIM)

## LiveImage -- Constitutes a small RAMDISK ISO, and is also merged with the regular BootCD
# Create the file list
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/livecd.cmake.lst "")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/livecd.cmake.lst "${CMAKE_CURRENT_BINARY_DIR}/empty\n")

# Create TEMP dir
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.cmake.lst "reactos/TEMP=${CMAKE_CURRENT_BINARY_DIR}/empty\n")

# Create user profile directories
add_allusers_profile_dirs(${CMAKE_CURRENT_BINARY_DIR}/livecd.cmake.lst "Profiles")
add_user_profile_dirs(${CMAKE_CURRENT_BINARY_DIR}/livecd.cmake.lst "Profiles" "Default User")

add_custom_target(livecd
    COMMAND native-mkisofs -quiet -o ${REACTOS_BINARY_DIR}/liveimg.iso -iso-level 4
        -publisher ${ISO_MANUFACTURER} -preparer ${ISO_MANUFACTURER} -volid ${ISO_VOLNAME} -volset ${ISO_VOLNAME}
        -duplicates-once -no-cache-inodes -graft-points -path-list ${CMAKE_CURRENT_BINARY_DIR}/livecd.$<CONFIG>.lst
    DEPENDS native-mkisofs
    VERBATIM)

if(DEFINED EFI_PLATFORM_ID)
    # For things like flashing USB drives, we also add the efi file into efi/boot.
    add_cd_file(TARGET efisys FILE ${CMAKE_CURRENT_BINARY_DIR}/efisys.bin DESTINATION loader NO_CAB FOR bootcd regtest)

    add_cd_file(
        TARGET uefildr
        DESTINATION efi/boot
        NO_CAB
        NAME_ON_CD boot${EFI_PLATFORM_ID}.efi
        FOR bootcd)
endif()
