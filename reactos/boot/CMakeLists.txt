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
    message(FATAL_ERROR "Unknown ARCH, can't generate a valid UEFI boot filename.")
endif()

add_custom_target(efisys
    COMMAND native-fatten ${CMAKE_CURRENT_BINARY_DIR}/efisys.bin -format 2880 EFIBOOT -boot ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/fat.bin -mkdir EFI -mkdir EFI/BOOT -add $<TARGET_FILE:bootmgfw> EFI/BOOT/boot${EFI_PLATFORM_ID}.efi
    DEPENDS native-fatten fat bootmgfw bcd_hive
    VERBATIM)

##bootcd
#clear it out
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bootcd.lst "")

if(USE_MKISOFS)
    add_custom_target(bootcd
        COMMAND native-mkisofs -o ${REACTOS_BINARY_DIR}/bootcd.iso -iso-level 4 -volid "ReactOS" -path-list ${CMAKE_CURRENT_BINARY_DIR}/bootcd.lst -graft-points -no-cache-inodes -eltorito-boot loader/isoboot.bin -no-emul-boot -boot-load-size 4 -eltorito-alt-boot -eltorito-platform efi -eltorito-boot loader/efisys.bin -hide boot.catalog -publisher "ReactOS Foundation" -preparer "ReactOS Foundation"
        DEPENDS native-mkisofs
        VERBATIM)
else()
    add_custom_target(bootcd
        COMMAND native-cdmake -j -m -bootdata:2\#p0,e,b${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isoboot.bin\#pEF,e,b${CMAKE_CURRENT_BINARY_DIR}/efisys.bin @${CMAKE_CURRENT_BINARY_DIR}/bootcd.lst REACTOS ${REACTOS_BINARY_DIR}/bootcd.iso
        DEPENDS native-cdmake efisys
        VERBATIM)
endif()

##bootcdregtest
#clear it out
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bootcdregtest.lst "")

if(USE_MKISOFS)
    add_custom_target(bootcdregtest
        COMMAND native-mkisofs -o ${REACTOS_BINARY_DIR}/bootcdregtest.iso -iso-level 4 -volid "ReactOS" -path-list ${CMAKE_CURRENT_BINARY_DIR}/bootcdregtest.lst -graft-points -no-cache-inodes -eltorito-boot loader/isobtrt.bin -no-emul-boot -boot-load-size 4 -eltorito-alt-boot -eltorito-platform efi -eltorito-boot loader/efisys.bin -hide boot.catalog -publisher "ReactOS Foundation" -preparer "ReactOS Foundation"
        DEPENDS native-mkisofs
        VERBATIM)
else()
    add_custom_target(bootcdregtest
        COMMAND native-cdmake -j -m -bootdata:2\#p0,e,b${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isobtrt.bin\#pEF,e,b${CMAKE_CURRENT_BINARY_DIR}/efisys.bin @${CMAKE_CURRENT_BINARY_DIR}/bootcdregtest.lst REACTOS ${REACTOS_BINARY_DIR}/bootcdregtest.iso
        DEPENDS native-cdmake efisys
        VERBATIM)
endif()

##livecd
#clear it out
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "")

if(USE_MKISOFS)
    #create the empty Desktop, Favorites, and Start Menu folders
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Desktop=${REACTOS_SOURCE_DIR}/boot/bootdata/empty\n")
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Favorites=${REACTOS_SOURCE_DIR}/boot/bootdata/empty\n")
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Start Menu/Programs=${REACTOS_SOURCE_DIR}/boot/bootdata/empty\n")

    add_custom_target(livecd
        COMMAND native-mkisofs -o ${REACTOS_BINARY_DIR}/livecd.iso -iso-level 4 -volid "ReactOS" -path-list ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst -graft-points -no-cache-inodes -eltorito-boot loader/isoboot.bin -no-emul-boot -boot-load-size 4 -eltorito-alt-boot -eltorito-platform efi -eltorito-boot loader/efisys.bin -hide boot.catalog -publisher "ReactOS Foundation" -preparer "ReactOS Foundation"
        DEPENDS native-mkisofs
        VERBATIM)
else()
    #create the empty Desktop, Favorites, and Start Menu folders
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Desktop\n")
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Favorites\n")
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Start Menu/Programs\n")

    add_custom_target(livecd
        COMMAND native-cdmake -j -m -bootdata:2\#p0,e,b${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isoboot.bin\#pEF,e,b${CMAKE_CURRENT_BINARY_DIR}/efisys.bin @${CMAKE_CURRENT_BINARY_DIR}/livecd.lst REACTOS ${REACTOS_BINARY_DIR}/livecd.iso
        DEPENDS native-cdmake efisys
        VERBATIM)
endif()

##hybridcd
#clear it out
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "")

if(USE_MKISOFS)
    #create the empty Desktop, Favorites, and Start Menu folders
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "livecd/Profiles/Default User/Desktop=${REACTOS_SOURCE_DIR}/boot/bootdata/empty\n")
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "livecd/Profiles/Default User/Favorites=${REACTOS_SOURCE_DIR}/boot/bootdata/empty\n")
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "livecd/Profiles/Default User/Start Menu/Programs=${REACTOS_SOURCE_DIR}/boot/bootdata/empty\n")

    add_custom_target(hybridcd
        COMMAND native-mkisofs -o ${REACTOS_BINARY_DIR}/hybridcd.iso -iso-level 4 -volid "ReactOS" -path-list ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst -graft-points -no-cache-inodes -eltorito-boot loader/isoboot.bin -no-emul-boot -boot-load-size 4 -eltorito-alt-boot -eltorito-platform efi -eltorito-boot loader/efisys.bin -hide boot.catalog -publisher "ReactOS Foundation" -preparer "ReactOS Foundation"
        DEPENDS native-mkisofs bootcd livecd
        VERBATIM)

    add_cd_file(TARGET efisys FILE ${CMAKE_CURRENT_BINARY_DIR}/efisys.bin DESTINATION loader NO_CAB FOR bootcd regtest livecd hybridcd)
else()
    #create the empty Desktop, Favorites, and Start Menu folders
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "livecd/Profiles/Default User/Desktop\n")
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "livecd/Profiles/Default User/Favorites\n")
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "livecd/Profiles/Default User/Start Menu/Programs\n")

    add_custom_target(hybridcd
        COMMAND native-cdmake -j -m -bootdata:2\#p0,e,b${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isoboot.bin\#pEF,e,b${CMAKE_CURRENT_BINARY_DIR}/efisys.bin @${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst REACTOS ${REACTOS_BINARY_DIR}/hybridcd.iso
        DEPENDS native-cdmake efisys bootcd livecd
        VERBATIM)
endif()

add_subdirectory(freeldr)
add_subdirectory(bootdata)
add_subdirectory(environ)
