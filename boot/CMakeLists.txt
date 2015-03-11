##bootcd
#clear it out
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bootcd.lst "")

add_custom_target(bootcd
    COMMAND native-cdmake -j -m -b ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isoboot.bin @${CMAKE_CURRENT_BINARY_DIR}/bootcd.lst REACTOS ${REACTOS_BINARY_DIR}/bootcd.iso
    DEPENDS native-cdmake
    VERBATIM)

##bootcdregtest
#clear it out
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bootcdregtest.lst "")

add_custom_target(bootcdregtest
    COMMAND native-cdmake -j -m -b ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isobtrt.bin @${CMAKE_CURRENT_BINARY_DIR}/bootcdregtest.lst REACTOS ${REACTOS_BINARY_DIR}/bootcdregtest.iso
    DEPENDS native-cdmake
    VERBATIM)

##livecd
#clear it out
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "")

#create the empty Desktop and Start Menu folders
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Desktop\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/livecd.lst "Profiles/Default User/Start Menu/Programs\n")

add_custom_target(livecd
    COMMAND native-cdmake -j -m -b ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isoboot.bin @${CMAKE_CURRENT_BINARY_DIR}/livecd.lst REACTOS ${REACTOS_BINARY_DIR}/livecd.iso
    DEPENDS native-cdmake
    VERBATIM)

##hybridcd
#clear it out
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "")

#create the empty Desktop and Start Menu folders
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "livecd/Profiles/Default User/Desktop\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst "livecd/Profiles/Default User/Start Menu/Programs\n")

add_custom_target(hybridcd
    COMMAND native-cdmake -j -m -b ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isoboot.bin @${CMAKE_CURRENT_BINARY_DIR}/hybridcd.lst REACTOS ${REACTOS_BINARY_DIR}/hybridcd.iso
    DEPENDS native-cdmake bootcd livecd
    VERBATIM)

add_subdirectory(freeldr)
add_subdirectory(bootdata)
