
function(add_iso _target)
    cmake_parse_arguments(_ISO "EXCLUDE_FROM_ALL" "ROOT" "" ${ARGN})

    if(NOT _ISO_EXCLUDE_FROM_ALL)
        set(_all "ALL")
    else()
        set(_all)
    endif()

    set(_empty ${CMAKE_CURRENT_BINARY_DIR}/.empty)
    set(_root ${_empty})
    file(MAKE_DIRECTORY ${_empty})
    if(_ISO_ROOT)
        set(_root ${_ISO_ROOT})
    endif()

    add_custom_target(${_target}_iso ${_all}
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_target}.iso)

    set(_filelist ${CMAKE_CURRENT_BINARY_DIR}/${_target})
    set_target_properties(${_target}_iso PROPERTIES
            _filelist ${_filelist}
            _empty ${_empty}
            _root ${_root}
            ISO_MANUFACTURER "ReactOS Foundation"
            ISO_VOLNAME "ReactOS"
            ISO_VOLSET "ReactOS")

    file(WRITE "${_filelist}.cmake" "$<TARGET_PROPERTY:${_target}_iso,_root>\n")
    file(GENERATE
         OUTPUT ${_filelist}.$<CONFIG>.lst
         INPUT  ${_filelist}.cmake)

    set(_tgt "TARGET_PROPERTY:${_target}_iso")

    # TODO: -sort file support?
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_target}.iso
        COMMAND native-mkisofs -quiet -o ${CMAKE_CURRENT_BINARY_DIR}/${_target}.iso -iso-level 4
            -publisher "$<${_tgt},ISO_MANUFACTURER>" -preparer "$<${_tgt},ISO_MANUFACTURER>"
            -volid "$<${_tgt},ISO_VOLNAME>" -volset "$<${_tgt},ISO_VOLSET>"
            -eltorito-boot loader/isoboot.bin -no-emul-boot -boot-load-size 4 -eltorito-alt-boot -eltorito-platform efi -eltorito-boot loader/efisys.bin -no-emul-boot -hide boot.catalog
            -no-cache-inodes -graft-points -path-list ${_filelist}.$<CONFIG>.lst
        COMMAND native-isohybrid -b ${CMAKE_BINARY_DIR}/boot/freeldr/bootsect/isombr.bin -t 0x96 ${CMAKE_CURRENT_BINARY_DIR}/${_target}.iso
        DEPENDS isombr native-isohybrid native-mkisofs ${_filelist}.$<CONFIG>.lst
        DEPENDS ${CMAKE_BINARY_DIR}/boot/freeldr/bootsect/isombr.bin
        DEPENDS "$<TARGET_PROPERTY:${_target}_iso,_iso_depends>"
        VERBATIM)

    #add_iso_file(${_target} FILE ${CMAKE_BINARY_DIR}/boot/freeldr/bootsect/isoboot.bin DESTINATION loader)
    #add_iso_file(${_target} FILE ${CMAKE_BINARY_DIR}/boot/freeldr/bootsect/dosmbr.bin DESTINATION loader)
    #add_iso_file(${_target} FILE ${CMAKE_BINARY_DIR}/boot/freeldr/bootsect/ext2.bin DESTINATION loader)
    #add_iso_file(${_target} FILE ${CMAKE_BINARY_DIR}/boot/freeldr/bootsect/fat.bin DESTINATION loader)
    #add_iso_file(${_target} FILE ${CMAKE_BINARY_DIR}/boot/freeldr/bootsect/fat32.bin DESTINATION loader)
    #add_iso_file(${_target} FILE ${CMAKE_BINARY_DIR}/boot/efisys.bin DESTINATION loader)

endfunction()

function(add_iso_file _target)
    cmake_parse_arguments(_ISO "" "FILE;TARGET;DESTINATION;NAME" "" ${ARGN})

    if(NOT (_ISO_TARGET OR _ISO_FILE))
        message(FATAL_ERROR "You must provide either target or a file to install!")
    endif()

    if(_ISO_TARGET AND _ISO_FILE)
        message(FATAL_ERROR "You must provide either target or a file to install!")
    endif()

    if(_ISO_TARGET)
        set(_file "$<TARGET_FILE:${_ISO_TARGET}>")
        set(_name "$<TARGET_FILE_NAME:${_ISO_TARGET}>")
        set_property(TARGET ${_target}_iso APPEND PROPERTY _iso_depends ${_ISO_TARGET})
    else()
        set(_file "${_ISO_FILE}")
        get_filename_component(_name ${_file} NAME)
        set_property(TARGET ${_target}_iso APPEND PROPERTY _iso_depends ${_ISO_FILE})
    endif()

    if(_ISO_NAME)
        set(_name "${_ISO_NAME}")
    endif()

    get_property(_filelist TARGET ${_target}_iso PROPERTY _filelist)
    file(APPEND ${_filelist}.cmake "${_ISO_DESTINATION}/${_name}=${_file}\n")
endfunction()

function(add_iso_folder _target _folder)
    get_property(_filelist TARGET ${_target}_iso PROPERTY _filelist)
    get_property(_empty TARGET ${_target}_iso PROPERTY _empty)
    file(APPEND ${_filelist}.cmake "${_folder}=${_empty}\n")
endfunction()
