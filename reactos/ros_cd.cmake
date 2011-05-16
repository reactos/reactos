#reactos.dff
add_custom_command(
    OUTPUT ${REACTOS_BINARY_DIR}/boot/reactos.dff
    COMMAND ${CMAKE_COMMAND} -E copy ${REACTOS_SOURCE_DIR}/boot/bootdata/packages/reactos.dff.in ${REACTOS_BINARY_DIR}/boot/reactos.dff
    DEPENDS ${REACTOS_SOURCE_DIR}/boot/bootdata/packages/reactos.dff.in)
    
file(STRINGS ${REACTOS_BINARY_DIR}/boot/ros_cab_target.txt CAB_TARGET_ENTRIES)
foreach(ENTRY ${CAB_TARGET_ENTRIES})
    string(REGEX REPLACE "^(.*)\t.*" "\\1" _targetname ${ENTRY})
    string(REGEX REPLACE "^.*\t(.)" "\\1" _dir_num ${ENTRY})
    get_target_property(_FILENAME ${_targetname} LOCATION)
    if(NOT CMAKE_HOST_SYSTEM_NAME MATCHES Windows)
        set(_FILENAME '\"${_FILENAME}\"')
    endif()
    add_custom_command(
        OUTPUT ${REACTOS_BINARY_DIR}/boot/reactos.dff
        COMMAND ${CMAKE_COMMAND} -E echo ${_FILENAME} ${_dir_num} >> ${REACTOS_BINARY_DIR}/boot/reactos.dff
        DEPENDS ${_targetname}
        APPEND)
endforeach()

file(STRINGS ${REACTOS_BINARY_DIR}/boot/ros_cab.txt CAB_TARGET_ENTRIES)
foreach(ENTRY ${CAB_TARGET_ENTRIES})
    string(REGEX REPLACE "^(.*)\t.*" "\\1" _FILENAME ${ENTRY})
    string(REGEX REPLACE "^.*\t(.)" "\\1" _dir_num ${ENTRY})
    if(NOT CMAKE_HOST_SYSTEM_NAME MATCHES Windows)
        set(QUOTED_FILENAME '\"${_FILENAME}\"')
    else()
        set(QUOTED_FILENAME ${_FILENAME})
    endif()
    add_custom_command(
        OUTPUT ${REACTOS_BINARY_DIR}/boot/reactos.dff
        COMMAND ${CMAKE_COMMAND} -E echo ${QUOTED_FILENAME} ${_dir_num} >> ${REACTOS_BINARY_DIR}/boot/reactos.dff
        DEPENDS ${_FILENAME}
        APPEND)
endforeach()

#reactos.cab
add_custom_command(
    OUTPUT ${REACTOS_BINARY_DIR}/boot/reactos.inf
    COMMAND native-cabman -C ${REACTOS_BINARY_DIR}/boot/reactos.dff -L ${REACTOS_BINARY_DIR}/boot -I -P ${REACTOS_SOURCE_DIR}
    DEPENDS ${REACTOS_BINARY_DIR}/boot/reactos.dff)
add_custom_command(
    OUTPUT ${REACTOS_BINARY_DIR}/boot/reactos.cab
    COMMAND native-cabman -C ${REACTOS_BINARY_DIR}/boot/reactos.dff -RC ${REACTOS_BINARY_DIR}/boot/reactos.inf -L ${REACTOS_BINARY_DIR}/boot -N -P ${REACTOS_SOURCE_DIR}
    DEPENDS ${REACTOS_BINARY_DIR}/boot/reactos.inf)

#bootcd target
macro(create_bootcd_dir BOOTCD_DIR _target)

    file(MAKE_DIRECTORY
        "${BOOTCD_DIR}"
        "${BOOTCD_DIR}/loader"
        "${BOOTCD_DIR}/reactos"
        "${BOOTCD_DIR}/reactos/system32")

    file(STRINGS ${REACTOS_BINARY_DIR}/boot/ros_minicd_target.txt MINICD_TARGET_ENTRIES)
    foreach(ENTRY ${MINICD_TARGET_ENTRIES})
        string(REGEX REPLACE "^(.*)\t.*\t.*" "\\1" _targetname ${ENTRY})
        string(REGEX REPLACE "^.*\t(.*)\t.*" "\\1" _DIR ${ENTRY})
        string(REGEX REPLACE "^.*\t.*\t(.*)"  "\\1"_NAMEONCD ${ENTRY})
        get_target_property(_FILENAME ${_targetname} LOCATION)
        set(filename ${BOOTCD_DIR}/${_DIR}/${_NAMEONCD})
        list(APPEND ${_target}_FILES ${filename})
        add_custom_command(
            OUTPUT ${filename}
            COMMAND ${CMAKE_COMMAND} -E copy ${_FILENAME} ${filename}
            DEPENDS ${_targetname})
    endforeach()

    file(STRINGS ${REACTOS_BINARY_DIR}/boot/ros_minicd.txt MINICD_ENTRIES)
    foreach(ENTRY ${MINICD_ENTRIES})
        string(REGEX REPLACE "^(.*)\t.*\t.*" "\\1" _FILENAME ${ENTRY})
        string(REGEX REPLACE "^.*\t(.*)\t.*" "\\1" _DIR ${ENTRY})
        string(REGEX REPLACE "^.*\t.*\t(.*)"  "\\1"_NAMEONCD ${ENTRY})
        set(filename ${BOOTCD_DIR}/${_DIR}/${_NAMEONCD})
        list(APPEND ${_target}_FILES ${filename})
        add_custom_command(
            OUTPUT ${filename}
            COMMAND ${CMAKE_COMMAND} -E copy ${_FILENAME} ${filename}
            DEPENDS ${_FILENAME})
    endforeach()
    
    add_custom_command(
        OUTPUT ${BOOTCD_DIR}/reactos/reactos.inf ${BOOTCD_DIR}/reactos/reactos.cab
        COMMAND ${CMAKE_COMMAND} -E copy ${REACTOS_BINARY_DIR}/boot/reactos.inf ${BOOTCD_DIR}/reactos/reactos.inf
        COMMAND ${CMAKE_COMMAND} -E copy ${REACTOS_BINARY_DIR}/boot/reactos.cab ${BOOTCD_DIR}/reactos/reactos.cab
        DEPENDS ${REACTOS_BINARY_DIR}/boot/reactos.cab)
    list(APPEND ${_target}_FILES ${filename} ${BOOTCD_DIR}/reactos/reactos.inf ${BOOTCD_DIR}/reactos/reactos.cab)
endmacro()

create_bootcd_dir(${REACTOS_BINARY_DIR}/boot/bootcd bootcd)
add_custom_target(bootcd 
    COMMAND native-cdmake -v -j -m -b ${CMAKE_CURRENT_BINARY_DIR}/boot/freeldr/bootsect/isoboot.bin ${BOOTCD_DIR} REACTOS ${REACTOS_BINARY_DIR}/bootcd.iso
    DEPENDS ${bootcd_FILES})
add_dependencies(bootcd dosmbr ext2 fat32 fat isoboot isobtrt vgafonts)
set_directory_properties(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${REACTOS_BINARY_DIR}/bootcd.iso)

#bootcdregtest target
create_bootcd_dir(${REACTOS_BINARY_DIR}/boot/bootcdregtest bootcdregtest)
add_custom_command(
    OUTPUT ${REACTOS_BINARY_DIR}/boot/bootcdregtest/reactos/unattend.inf
    COMMAND ${CMAKE_COMMAND} -E copy ${REACTOS_SOURCE_DIR}/boot/bootdata/bootcdregtest/unattend.inf ${REACTOS_BINARY_DIR}/boot/bootcdregtest/reactos/unattend.inf
    DEPENDS ${REACTOS_SOURCE_DIR}/boot/bootdata/bootcdregtest/unattend.inf ${REACTOS_BINARY_DIR}/boot/bootcdregtest)
add_custom_target(bootcdregtest
    COMMAND native-cdmake -v -j -m -b ${CMAKE_CURRENT_BINARY_DIR}/boot/freeldr/bootsect/isoboot.bin ${REACTOS_BINARY_DIR}/boot/bootcdregtest REACTOS ${REACTOS_BINARY_DIR}/bootcdregtest.iso
    DEPENDS ${REACTOS_BINARY_DIR}/boot/bootcdregtest/reactos/unattend.inf ${bootcdregtest_FILES})
add_dependencies(bootcdregtest dosmbr ext2 fat32 fat isoboot isobtrt vgafonts)


#livecd target
file(MAKE_DIRECTORY
    "${LIVECD_DIR}"
    "${LIVECD_DIR}/loader"
    "${LIVECD_DIR}/Profiles"
    "${LIVECD_DIR}/Profiles/All Users"
    "${LIVECD_DIR}/Profiles/All Users/Desktop"
    "${LIVECD_DIR}/Profiles/Default User"
    "${LIVECD_DIR}/Profiles/Default User/Desktop"
    "${LIVECD_DIR}/Profiles/Default User/My Documents"
    "${LIVECD_DIR}/reactos"
    "${LIVECD_DIR}/reactos/inf"
    "${LIVECD_DIR}/reactos/fonts"
    "${LIVECD_DIR}/reactos/system32"
    "${LIVECD_DIR}/reactos/system32/config")

file(STRINGS ${REACTOS_BINARY_DIR}/boot/ros_livecd_target.txt LIVECD_TARGET_ENTRIES)
foreach(ENTRY ${LIVECD_TARGET_ENTRIES})
    string(REGEX REPLACE "^(.*)\t.*\t.*" "\\1" _targetname ${ENTRY})
    string(REGEX REPLACE "^.*\t(.*)\t.*" "\\1" _DIR ${ENTRY})
    string(REGEX REPLACE "^.*\t.*\t(.*)"  "\\1"_NAMEONCD ${ENTRY})
    get_target_property(_FILENAME ${_targetname} LOCATION)
    set(filename ${LIVECD_DIR}/${_DIR}/${_NAMEONCD})
    list(APPEND LIVECD_FILES ${filename})
    add_custom_command(
        OUTPUT ${filename}
        COMMAND ${CMAKE_COMMAND} -E copy ${_FILENAME} ${LIVECD_DIR}/${_DIR}/${_NAMEONCD}
        DEPENDS ${_targetname})
endforeach()
file(STRINGS ${REACTOS_BINARY_DIR}/boot/ros_livecd.txt LIVECD_ENTRIES)

foreach(ENTRY ${LIVECD_ENTRIES})
    string(REGEX REPLACE "^(.*)\t.*\t.*" "\\1" _FILENAME ${ENTRY})
    string(REGEX REPLACE "^.*\t(.*)\t.*" "\\1" _DIR ${ENTRY})
    string(REGEX REPLACE "^.*\t.*\t(.*)"  "\\1"_NAMEONCD ${ENTRY})
    set(filename ${LIVECD_DIR}/${_DIR}/${_NAMEONCD})
    list(APPEND LIVECD_FILES ${filename})
    add_custom_command(
        OUTPUT ${filename}
        COMMAND ${CMAKE_COMMAND} -E copy ${_FILENAME} ${LIVECD_DIR}/${_DIR}/${_NAMEONCD}
        DEPENDS ${_FILENAME})
endforeach()

add_custom_target(livecd
    COMMAND native-cdmake -v -j -m -b ${CMAKE_CURRENT_BINARY_DIR}/boot/freeldr/bootsect/isoboot.bin ${LIVECD_DIR} REACTOS ${REACTOS_BINARY_DIR}/livecd.iso
    DEPENDS ${LIVECD_FILES})
add_dependencies(livecd isoboot livecd_hives vgafonts)

set_directory_properties(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${REACTOS_BINARY_DIR}/livecd.iso)
