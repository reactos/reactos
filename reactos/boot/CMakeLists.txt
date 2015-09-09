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
    COMMAND native-fatten ${CMAKE_CURRENT_BINARY_DIR}/efisys.bin -format 2880 -boot ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/fat.bin -mkdir EFI -mkdir EFI/BOOT -add $<TARGET_FILE:bootmgfw> EFI/BOOT/boot${EFI_PLATFORM_ID}.efi
    DEPENDS native-fatten bootmgfw fat
    VERBATIM)

##bootcd
#clear it out
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bootcd.lst "")

add_custom_target(bootcd
    COMMAND native-cdmake -j -m -bootdata:2\#p0,e,b${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isoboot.bin\#pEF,e,b${CMAKE_CURRENT_BINARY_DIR}/efisys.bin @${CMAKE_CURRENT_BINARY_DIR}/bootcd.lst REACTOS ${REACTOS_BINARY_DIR}/bootcd.iso
    DEPENDS native-cdmake efisys
    VERBATIM)

##bootcdregtest
#clear it out
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bootcdregtest.lst "")

add_custom_target(bootcdregtest
    COMMAND native-cdmake -j -m -bootdata:2\#p0,e,b${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isobtrt.bin\#pEF,e,b${CMAKE_CURRENT_BINARY_DIR}/efisys.bin @${CMAKE_CURRENT_BINARY_DIR}/bootcdregtest.lst REACTOS ${REACTOS_BINARY_DIR}/bootcdregtest.iso
    DEPENDS native-cdmake efisys
    VERBATIM)

##livecd
#clear it out
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "")

#create the empty Desktop, Favorites, and Start Menu folders
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Desktop\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Favorites\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Start Menu/Programs\n")

add_custom_target(livecd
    COMMAND native-cdmake -j -m -bootdata:2\#p0,e,b${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isoboot.bin\#pEF,e,b${CMAKE_CURRENT_BINARY_DIR}/efisys.bin @${CMAKE_CURRENT_BINARY_DIR}/livecd.lst REACTOS ${REACTOS_BINARY_DIR}/livecd.iso
    DEPENDS native-cdmake efisys
    VERBATIM)

##hybridcd
#clear it out
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "")

#create the empty Desktop, Favorites, and Start Menu folders
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "livecd/Profiles/Default User/Desktop\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "Profiles/Default User/Favorites\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "livecd/Profiles/Default User/Start Menu/Programs\n")

add_custom_target(hybridcd
    COMMAND native-cdmake -j -m -bootdata:2\#p0,e,b${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isoboot.bin\#pEF,e,b${CMAKE_CURRENT_BINARY_DIR}/efisys.bin @${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst REACTOS ${REACTOS_BINARY_DIR}/hybridcd.iso
    DEPENDS native-cdmake efisys bootcd livecd
    VERBATIM)

add_subdirectory(freeldr)
add_subdirectory(bootdata)
add_subdirectory(environ)
