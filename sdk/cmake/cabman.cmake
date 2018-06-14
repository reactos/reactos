

function(add_cabinet _target _dff)
    cmake_parse_arguments(_CAB "EXCLUDE_FROM_ALL" "" "" ${ARGN})

    if(NOT _CAB_EXCLUDE_FROM_ALL)
        set(_all "ALL")
    else()
        set(_all)
    endif()

    add_custom_target(${_target}_cab ${_all}
        SOURCES ${_dff}
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_target}.cab
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_target}.inf)

    set(_dff_file ${CMAKE_CURRENT_BINARY_DIR}/${_target})

    set_property(TARGET ${_target}_cab PROPERTY _dff_file ${_dff_file})

    # hack: make dff file changes re-run cmake by configuring it
    #configure_file(${_dff} ${CMAKE_CURRENT_BINARY_DIR}/.${_target}.dummy)

    file(GENERATE
         OUTPUT ${_dff_file}.$<CONFIG>.dyn
         INPUT  ${_dff_file}.dff.cmake)

    file(WRITE ${_dff_file}.dff.cmake "")

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_target}.cab ${CMAKE_CURRENT_BINARY_DIR}/${_target}.inf
        COMMAND ${CMAKE_COMMAND} -D SRC1=${_dff}
                                 -D SRC2=${_dff_file}.$<CONFIG>.dyn
                                 -D DST=${CMAKE_CURRENT_BINARY_DIR}/${_target}.$<CONFIG>.dff
                                 -P ${REACTOS_SOURCE_DIR}/sdk/cmake/concat.cmake
        COMMAND native-cabman -C ${CMAKE_CURRENT_BINARY_DIR}/${_target}.$<CONFIG>.dff -I -P ${REACTOS_SOURCE_DIR}
        COMMAND native-cabman -C ${CMAKE_CURRENT_BINARY_DIR}/${_target}.$<CONFIG>.dff -N -P ${REACTOS_SOURCE_DIR} -RC ${CMAKE_CURRENT_BINARY_DIR}/${_target}.inf
        DEPENDS native-cabman ${_dff} ${_dff_file}.$<CONFIG>.dyn
        DEPENDS "$<TARGET_PROPERTY:${_target}_cab,_cab_file_depends>")

    _cabman_parse_dff(${_target}_cab ${_dff})
endfunction()

function(add_cab_file _target)
    cmake_parse_arguments(_CAB "OPTIONAL" "DESTINATION" "FILE;TARGET" ${ARGN})

    if(NOT (_CAB_TARGET OR _CAB_FILE))
        message(FATAL_ERROR "You must provide a target or a file to install!")
    endif()

    if(NOT _CAB_DESTINATION)
        message(FATAL_ERROR "You must provide a destination")
    endif()

    get_property(_dff_file TARGET ${_target}_cab PROPERTY _dff_file)

    _cabman_path_to_num(${_target} "${_CAB_DESTINATION}" _num)
    if(${_num} EQUAL -1)
        message(FATAL_ERROR "Destination ${_CAB_DESTINATION} not defined in directive file")
    endif()

    if(_CAB_OPTIONAL)
        set(_optional " optional")
    else()
        set(_optional "")
    endif()

    foreach(_cab_target ${_CAB_TARGET})
        file(APPEND ${_dff_file}.dff.cmake "\"$<TARGET_FILE:${_cab_target}>\" ${_num}${_optional}\n")
        set_property(TARGET ${_target}_cab APPEND PROPERTY _cab_file_depends ${_cab_target})
    endforeach()

    foreach(_file ${_CAB_FILE})
        file(APPEND ${_dff_file}.dff.cmake "\"${_file}\" ${_num}${_optional}\n")
        if(NOT _CAB_OPTIONAL)
            set_property(TARGET ${_target}_cab APPEND PROPERTY _cab_file_depends ${_file})
        endif()
    endforeach()
endfunction()

# Maps paths to their inf number, or return -1 if none exist
function(_cabman_path_to_num _target _path _out)
    get_property(_inf_directories TARGET ${_target}_cab PROPERTY _inf_directories)
    get_property(_inf_directories_num TARGET ${_target}_cab PROPERTY _inf_directories_num)

    list(FIND _inf_directories "${_path}" _num)
    if(${_num} EQUAL -1)
        set(${_out} -1 PARENT_SCOPE)
    else()
        list(GET _inf_directories_num ${_num} _num)
        set(${_out} ${_num} PARENT_SCOPE)
    endif()
endfunction()

function(_cabman_parse_dff _target _dff)
    set(section _none)

    file(STRINGS ${_dff} lines ENCODING UTF-8)
    foreach(line ${lines})
        # Strip comments
        list(GET line 0 line)

        if("${line}" MATCHES "^\\.Set ([a-zA-Z]+)=\"?([^\"]+)\"?")
            # Capture cabman variables
            set(var_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}")
        elseif("${line}" MATCHES "^\\[([a-zA-Z]+)\\]")
            set(section ${CMAKE_MATCH_1})
        elseif(section STREQUAL Directories
               AND "${line}" MATCHES "^([0-9]+) += +\"?([^\"]+)\"?")
            # Capture directory->number mappings
            string(REPLACE "\\" "/" _dir "${CMAKE_MATCH_2}")
            set_property(TARGET ${_target} APPEND PROPERTY _inf_directories ${_dir})
            set_property(TARGET ${_target} APPEND PROPERTY _inf_directories_num ${CMAKE_MATCH_1})
        endif()
    endforeach()
endfunction()
