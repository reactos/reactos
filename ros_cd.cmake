#reactos.dff
add_custom_command(
    OUTPUT ${REACTOS_BINARY_DIR}/boot/reactos.dff
    COMMAND ${CMAKE_COMMAND} -E copy ${REACTOS_SOURCE_DIR}/boot/bootdata/packages/reactos.dff.in ${REACTOS_BINARY_DIR}/boot/reactos.dff
    DEPENDS ${REACTOS_SOURCE_DIR}/boot/bootdata/packages/reactos.dff.in
)
    
file(STRINGS ${REACTOS_BINARY_DIR}/boot/ros_cab_target.txt CAB_TARGET_ENTRIES)
foreach(ENTRY ${CAB_TARGET_ENTRIES})
    string(REGEX REPLACE "^(.*)\t.*" "\\1" _targetname ${ENTRY})
    string(REGEX REPLACE "^.*\t(.)" "\\1" _dir_num ${ENTRY})
    get_target_property(_FILENAME ${_targetname} LOCATION)
    add_custom_command(
        OUTPUT ${REACTOS_BINARY_DIR}/boot/reactos.dff
        COMMAND ${CMAKE_COMMAND} -E echo ${_FILENAME} ${_dir_num} >> ${REACTOS_BINARY_DIR}/boot/reactos.dff
        DEPENDS ${_targetname}
        APPEND
    )
endforeach()

file(STRINGS ${REACTOS_BINARY_DIR}/boot/ros_cab.txt CAB_TARGET_ENTRIES)
foreach(ENTRY ${CAB_TARGET_ENTRIES})
    string(REGEX REPLACE "^(.*)\t.*" "\\1" _FILENAME ${ENTRY})
    string(REGEX REPLACE "^.*\t(.)" "\\1" _dir_num ${ENTRY})
        add_custom_command(
        OUTPUT ${REACTOS_BINARY_DIR}/boot/reactos.dff
        COMMAND ${CMAKE_COMMAND} -E echo ${_FILENAME} ${_dir_num} >> ${REACTOS_BINARY_DIR}/boot/reactos.dff
        DEPENDS ${_FILENAME}
        APPEND
    )
endforeach()

#reactos.cab
add_custom_command(
    OUTPUT ${REACTOS_BINARY_DIR}/bootcd/reactos/reactos.inf
    COMMAND native-cabman -C ${REACTOS_BINARY_DIR}/boot/reactos.dff -L ${REACTOS_BINARY_DIR}/boot/bootcd/reactos -I
    DEPENDS ${REACTOS_BINARY_DIR}/boot/reactos.dff
)
add_custom_command(
    OUTPUT ${REACTOS_BINARY_DIR}/bootcd/reactos/reactos.cab
    COMMAND native-cabman -C ${REACTOS_BINARY_DIR}/boot/reactos.dff -RC ${REACTOS_BINARY_DIR}/boot/bootcd/reactos/reactos.inf -L ${REACTOS_BINARY_DIR}/boot/bootcd/reactos -N
    DEPENDS ${REACTOS_BINARY_DIR}/bootcd/reactos/reactos.inf
)
list( APPEND BOOTCD_FILES ${REACTOS_BINARY_DIR}/bootcd/reactos/reactos.cab)

#bootcd target
set(BOOTCD_DIR "${REACTOS_BINARY_DIR}/boot/bootcd")

file(MAKE_DIRECTORY "${BOOTCD_DIR}")
file(MAKE_DIRECTORY "${BOOTCD_DIR}/loader")
file(MAKE_DIRECTORY "${BOOTCD_DIR}/reactos")
file(MAKE_DIRECTORY "${BOOTCD_DIR}/reactos/system32")

file(STRINGS ${REACTOS_BINARY_DIR}/boot/ros_minicd_target.txt MINICD_TARGET_ENTRIES)
foreach(ENTRY ${MINICD_TARGET_ENTRIES})
    string(REGEX REPLACE "^(.*)\t.*\t.*" "\\1" _targetname ${ENTRY})
    string(REGEX REPLACE "^.*\t(.*)\t.*" "\\1" _DIR ${ENTRY})
    string(REGEX REPLACE "^.*\t.*\t(.*)"  "\\1"_NAMEONCD ${ENTRY})
    get_target_property(_FILENAME ${_targetname} LOCATION)
    set(filename ${BOOTCD_DIR}/${_DIR}/${_NAMEONCD})
    list( APPEND BOOTCD_FILES ${filename})
    add_custom_command(
        OUTPUT ${filename}
        COMMAND ${CMAKE_COMMAND} -E copy ${_FILENAME} ${BOOTCD_DIR}/${_DIR}/${_NAMEONCD}
        DEPENDS ${_targetname}
    )
endforeach()

file(STRINGS ${REACTOS_BINARY_DIR}/boot/ros_minicd.txt MINICD_ENTRIES)
foreach(ENTRY ${MINICD_ENTRIES})
    string(REGEX REPLACE "^(.*)\t.*\t.*" "\\1" _FILENAME ${ENTRY})
    string(REGEX REPLACE "^.*\t(.*)\t.*" "\\1" _DIR ${ENTRY})
    string(REGEX REPLACE "^.*\t.*\t(.*)"  "\\1"_NAMEONCD ${ENTRY})
    set(filename ${BOOTCD_DIR}/${_DIR}/${_NAMEONCD})
    list( APPEND BOOTCD_FILES ${filename})
    add_custom_command(
        OUTPUT ${filename}
        COMMAND ${CMAKE_COMMAND} -E copy ${_FILENAME} ${BOOTCD_DIR}/${_DIR}/${_NAMEONCD}
        DEPENDS ${_FILENAME}
    )
endforeach()

add_custom_target(bootcd 
    COMMAND native-cdmake -v -j -m -b ${CMAKE_CURRENT_BINARY_DIR}/boot/freeldr/bootsect/isoboot.bin ${BOOTCD_DIR} REACTOS ${REACTOS_BINARY_DIR}/minicd.iso
    DEPENDS ${BOOTCD_FILES})
    
add_dependencies(bootcd dosmbr ext2 fat32 fat isoboot isobtrt vgafonts)

set_directory_properties(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${REACTOS_BINARY_DIR}/minicd.iso)

#livecd target
file(MAKE_DIRECTORY "${LIVECD_DIR}")
file(MAKE_DIRECTORY "${LIVECD_DIR}/loader")
file(MAKE_DIRECTORY "${LIVECD_DIR}/Profiles")
file(MAKE_DIRECTORY "${LIVECD_DIR}/Profiles/All Users")
file(MAKE_DIRECTORY "${LIVECD_DIR}/Profiles/All Users/Desktop")
file(MAKE_DIRECTORY "${LIVECD_DIR}/Profiles/Default User")
file(MAKE_DIRECTORY "${LIVECD_DIR}/Profiles/Default User/Desktop")
file(MAKE_DIRECTORY "${LIVECD_DIR}/Profiles/Default User/My Documents")
file(MAKE_DIRECTORY "${LIVECD_DIR}/reactos")
file(MAKE_DIRECTORY "${LIVECD_DIR}/reactos/inf")
file(MAKE_DIRECTORY "${LIVECD_DIR}/reactos/fonts")
file(MAKE_DIRECTORY "${LIVECD_DIR}/reactos/system32")
file(MAKE_DIRECTORY "${LIVECD_DIR}/reactos/system32/config")

file(STRINGS ${REACTOS_BINARY_DIR}/boot/ros_livecd_target.txt LIVECD_TARGET_ENTRIES)
foreach(ENTRY ${LIVECD_TARGET_ENTRIES})
    string(REGEX REPLACE "^(.*)\t.*\t.*" "\\1" _targetname ${ENTRY})
    string(REGEX REPLACE "^.*\t(.*)\t.*" "\\1" _DIR ${ENTRY})
    string(REGEX REPLACE "^.*\t.*\t(.*)"  "\\1"_NAMEONCD ${ENTRY})
    get_target_property(_FILENAME ${_targetname} LOCATION)
    set(filename ${LIVECD_DIR}/${_DIR}/${_NAMEONCD})
    list( APPEND LIVECD_FILES ${filename})
    add_custom_command(
        OUTPUT ${filename}
        COMMAND ${CMAKE_COMMAND} -E copy ${_FILENAME} ${LIVECD_DIR}/${_DIR}/${_NAMEONCD}
        DEPENDS ${_targetname}
    )
endforeach()
file(STRINGS ${REACTOS_BINARY_DIR}/boot/ros_livecd.txt LIVECD_ENTRIES)

foreach(ENTRY ${LIVECD_ENTRIES})
    string(REGEX REPLACE "^(.*)\t.*\t.*" "\\1" _FILENAME ${ENTRY})
    string(REGEX REPLACE "^.*\t(.*)\t.*" "\\1" _DIR ${ENTRY})
    string(REGEX REPLACE "^.*\t.*\t(.*)"  "\\1"_NAMEONCD ${ENTRY})
    set(filename ${LIVECD_DIR}/${_DIR}/${_NAMEONCD})
    list( APPEND LIVECD_FILES ${filename})
    add_custom_command(
        OUTPUT ${filename}
        COMMAND ${CMAKE_COMMAND} -E copy ${_FILENAME} ${LIVECD_DIR}/${_DIR}/${_NAMEONCD}
        DEPENDS ${_FILENAME}
    )
endforeach()

add_custom_target(livecd
    COMMAND native-cdmake -v -j -m -b ${CMAKE_CURRENT_BINARY_DIR}/boot/freeldr/bootsect/isoboot.bin ${LIVECD_DIR} REACTOS ${REACTOS_BINARY_DIR}/livecd.iso
    DEPENDS ${LIVECD_FILES})
add_dependencies(livecd isoboot livecd_hives)

set_directory_properties(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${REACTOS_BINARY_DIR}/livecd.iso)