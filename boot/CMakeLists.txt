##bootcd
#clear it out
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/bootcd.cmake 
    "file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bootcd)\n")

add_custom_target(bootcd
    ${CMAKE_COMMAND} -D CD_DIR=${CMAKE_CURRENT_BINARY_DIR}/bootcd
                             -P ${CMAKE_CURRENT_BINARY_DIR}/bootcd.cmake
    COMMAND native-cdmake -v -j -m -b ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isoboot.bin ${CMAKE_CURRENT_BINARY_DIR}/bootcd REACTOS ${REACTOS_BINARY_DIR}/bootcd.iso
    VERBATIM)
    
##livecd
#clear it out
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/livecd.cmake 
    "file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/livecd)\n")

add_custom_target(livecd
    ${CMAKE_COMMAND} -D CD_DIR=${CMAKE_CURRENT_BINARY_DIR}/livecd
                             -P ${CMAKE_CURRENT_BINARY_DIR}/livecd.cmake
    COMMAND native-cdmake -v -j -m -b ${CMAKE_CURRENT_BINARY_DIR}/freeldr/bootsect/isoboot.bin ${CMAKE_CURRENT_BINARY_DIR}/livecd REACTOS ${REACTOS_BINARY_DIR}/livecd.iso
    VERBATIM)

add_subdirectory(freeldr)
add_subdirectory(bootdata)
