
if (NOT MSVC)

macro(CreateBootSectorTarget _target_name _asm_file _object_file)
    get_filename_component(OBJECT_PATH ${_object_file} PATH)
    get_filename_component(OBJECT_NAME ${_object_file} NAME)
    file(MAKE_DIRECTORY ${OBJECT_PATH})
    get_directory_property(defines COMPILE_DEFINITIONS)
    get_directory_property(includes INCLUDE_DIRECTORIES)

    foreach(arg ${defines})
        set(result_defs ${result_defs} -D${arg})
    endforeach()

    foreach(arg ${includes})
        set(result_incs -I${arg} ${result_incs})
    endforeach()

    add_custom_command(
        OUTPUT ${_object_file}
        COMMAND nasm -o ${_object_file} ${result_incs} ${result_defs} -f bin ${_asm_file}
        DEPENDS ${_asm_file})
    set_source_files_properties(${_object_file} PROPERTIES GENERATED TRUE)
    add_custom_target(${_target_name} ALL DEPENDS ${_object_file})
    add_minicd(${_object_file} loader ${OBJECT_NAME})
endmacro()

else()

macro(CreateBootSectorTarget _target_name _asm_file _object_file)
endmacro()

endif()

macro(add_minicd_target _targetname _dir) # optional parameter: _nameoncd
    if("${ARGN}" STREQUAL "")
        get_target_property(FILENAME ${_targetname} LOCATION)
        get_filename_component(_nameoncd ${FILENAME} NAME)
    else()
        set(_nameoncd ${ARGN})
    endif()

    file(APPEND ${REACTOS_BINARY_DIR}/boot/ros_minicd_target.txt "${_targetname}\t${_dir}\t${_nameoncd}\n")
endmacro()

macro(add_minicd FILENAME _dir _nameoncd)
    file(APPEND ${REACTOS_BINARY_DIR}/boot/ros_minicd.txt "${FILENAME}\t${_dir}\t${_nameoncd}\n")
endmacro()

macro(set_cpp)
    include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/c++/stlport)
    set(IS_CPP 1)
    add_definitions(
        -DNATIVE_CPP_INCLUDE=${REACTOS_SOURCE_DIR}/include/c++
        -DNATIVE_C_INCLUDE=${REACTOS_SOURCE_DIR}/include/crt)
endmacro()

macro(add_livecd_target _targetname _dir )# optional parameter : _nameoncd
    if("${ARGN}" STREQUAL "")
        get_target_property(FILENAME ${_targetname} LOCATION)
        get_filename_component(_nameoncd ${FILENAME} NAME)
    else()
        set(_nameoncd ${ARGN})
    endif()

    file(APPEND ${REACTOS_BINARY_DIR}/boot/ros_livecd_target.txt "${_targetname}\t${_dir}\t${_nameoncd}\n")
endmacro()

macro(add_livecd FILENAME _dir)# optional parameter : _nameoncd
    if("${ARGN}" STREQUAL "")
        get_filename_component(_nameoncd ${FILENAME} NAME)
    else()
        set(_nameoncd ${ARGN})
    endif()
    file(APPEND ${REACTOS_BINARY_DIR}/boot/ros_livecd.txt "${FILENAME}\t${_dir}\t${_nameoncd}\n")
endmacro()

macro(cab_to_dir _dir_num _var_name)
#   1 = system32
#   2 = system32\drivers
#   3 = Fonts
#   4 =
#   5 = system32\drivers\etc
#   6 = inf
#   7 = bin
#   8 = media
    if(${_dir_num} STREQUAL "1")
        set(${_var_name} "reactos/system32")
    elseif(${_dir_num} STREQUAL "2")
        set(${_var_name} "reactos/system32/drivers")
    elseif(${_dir_num} STREQUAL "3")
        set(${_var_name} "reactos/fonts")
    elseif(${_dir_num} STREQUAL "4")
        set(${_var_name} "reactos")
    elseif(${_dir_num} STREQUAL "5")
        set(${_var_name} "reactos/system32/drivers/etc")
    elseif(${_dir_num} STREQUAL "6")
        set(${_var_name} "reactos/inf")
    elseif(${_dir_num} STREQUAL "7")
        set(${_var_name} "reactos/bin")
    elseif(${_dir_num} STREQUAL "8")
        set(${_var_name} "reactos/system32/drivers")
    else()
        message(FATAL_ERROR "Wrong directory ${_dir_num}")
    endif()
endmacro()

macro(add_cab_target _targetname _num )
    file(APPEND ${REACTOS_BINARY_DIR}/boot/ros_cab_target.txt "${_targetname}\t${_num}\n")
    cab_to_dir(${_num} _dir)
    add_livecd_target(${_targetname} ${_dir})
endmacro()

macro(add_cab FILENAME _num)
    file(APPEND ${REACTOS_BINARY_DIR}/boot/ros_cab.txt "${FILENAME}\t${_num}\n")
    cab_to_dir(${_num} _dir)
    add_livecd(${FILENAME} ${_dir})
endmacro()

macro(add_dependency_node _node)
    if(GENERATE_DEPENDENCY_GRAPH)
        get_target_property(_type ${_node} TYPE)
        if(_type MATCHES SHARED_LIBRARY)
            file(APPEND ${REACTOS_BINARY_DIR}/dependencies.graphml "    <node id=\"${_node}\"/>\n")
        endif()
     endif()
endmacro()

macro(add_dependency_edge _source _target)
    if(GENERATE_DEPENDENCY_GRAPH)
        get_target_property(_type ${_source} TYPE)
        if(_type MATCHES SHARED_LIBRARY)
            file(APPEND ${REACTOS_BINARY_DIR}/dependencies.graphml "    <edge source=\"${_source}\" target=\"${_target}\"/>\n")
        endif()
    endif()
endmacro()

macro(add_dependency_header)
    file(APPEND ${REACTOS_BINARY_DIR}/dependencies.graphml "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<graphml>\n  <graph id=\"ReactOS dependencies\" edgedefault=\"directed\">\n")
    add_dependency_node(ntdll)
endmacro()

macro(add_dependency_footer)
    file(APPEND ${REACTOS_BINARY_DIR}/dependencies.graphml "  </graph>\n</graphml>\n")
endmacro()

macro(add_message_headers)
    foreach(_in_FILE ${ARGN})
        get_filename_component(FILE ${_in_FILE} NAME_WE)
        macro_mc(${FILE})
        add_custom_command(
            OUTPUT ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.rc ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.h
            COMMAND ${COMMAND_MC}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.mc)
        set_source_files_properties(
            ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.h ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.rc
            PROPERTIES GENERATED TRUE)
        add_custom_target(${FILE} ALL DEPENDS ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.h ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.rc)
    endforeach()
endmacro()

