
macro(set_cpp)
    include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/c++/stlport)
    set(IS_CPP 1)
    add_definitions(
        -DNATIVE_CPP_INCLUDE=${REACTOS_SOURCE_DIR}/include/c++
        -DNATIVE_C_INCLUDE=${REACTOS_SOURCE_DIR}/include/crt)
endmacro()

macro(add_dependency_node _node)
    if(GENERATE_DEPENDENCY_GRAPH)
        get_target_property(_type ${_node} TYPE)
        if(_type MATCHES SHARED_LIBRARY OR ${_node} MATCHES ntoskrnl)
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
endmacro()

macro(add_dependency_footer)
    add_dependency_node(ntdll)
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

macro(dir_to_num dir var)
    if(${dir} STREQUAL reactos/system32)
        set(${var} 1)
    elseif(${dir} STREQUAL reactos/system32/drivers)
        set(${var} 2)
    elseif(${dir} STREQUAL reactos/Fonts)
        set(${var} 3)
    elseif(${dir} STREQUAL reactos)
        set(${var} 4)
    elseif(${dir} STREQUAL reactos/system32/drivers/etc)
        set(${var} 5)
    elseif(${dir} STREQUAL reactos/inf)
        set(${var} 6)
    elseif(${dir} STREQUAL reactos/bin)
        set(${var} 7)
    elseif(${dir} STREQUAL reactos/media)
        set(${var} 8)
    else()
        message(ERROR "Wrong destination: ${dir}")
    endif()
endmacro()

function(add_cd_file)
    cmake_parse_arguments(_CD "NO_CAB" "DESTINATION;NAME_ON_CD;TARGET" "FILE;FOR" ${ARGN})
    if(NOT (_CD_TARGET OR _CD_FILE))
        message(FATAL_ERROR "You must provide a target or a file to install!")
    endif()

    if(NOT _CD_DESTINATION)
        message(FATAL_ERROR "You must provide a destination")
    elseif(${_CD_DESTINATION} STREQUAL root)
        set(_CD_DESTINATION "")
    endif()

    if(NOT _CD_FOR)
        message(FATAL_ERROR "You must provide a cd name (or "all" for all of them) to install the file on!")
    endif()

    #get file if we need to
    if(NOT _CD_FILE)
        get_target_property(_CD_FILE ${_CD_TARGET} LOCATION)
    endif()

    #do we add it to all CDs?
    if(_CD_FOR STREQUAL all)
        set(_CD_FOR "bootcd;livecd;regtest")
    endif()

    #do we add it to bootcd?
    list(FIND _CD_FOR bootcd __cd)
    if(NOT __cd EQUAL -1)
        #whether or not we should put it in reactos.cab or directly on cd
        if(_CD_NO_CAB)
            #directly on cd
            foreach(item ${_CD_FILE})
                file(APPEND ${REACTOS_BINARY_DIR}/boot/bootcd.cmake "file(COPY \"${item}\" DESTINATION \"\${CD_DIR}/${_CD_DESTINATION}\")\n")
            endforeach()
            if(_CD_NAME_ON_CD)
                get_filename_component(__file ${_CD_FILE} NAME)
                #rename it in the cd tree
                file(APPEND ${REACTOS_BINARY_DIR}/boot/bootcd.cmake "file(RENAME \${CD_DIR}/${_CD_DESTINATION}/${__file} \${CD_DIR}/${_CD_DESTINATION}/${_CD_NAME_ON_CD})\n")
            endif()
            if(_CD_TARGET)
                #manage dependency
                add_dependencies(bootcd ${_CD_TARGET})
            endif()
        else()
            #add it in reactos.cab
            dir_to_num(${_CD_DESTINATION} _num)
            if(CMAKE_HOST_SYSTEM_NAME MATCHES Windows)
                file(APPEND ${REACTOS_BINARY_DIR}/boot/bootdata/packages/reactos.dff.dyn "${_CD_FILE} ${_num}\n")
            else()
                file(APPEND ${REACTOS_BINARY_DIR}/boot/bootdata/packages/reactos.dff.dyn "\"${_CD_FILE}\" ${_num}\n")
            endif()
            if(_CD_TARGET)
                #manage dependency
                add_dependencies(reactos_cab ${_CD_TARGET})
            endif()
        endif()
    endif() #end bootcd

    #do we add it to livecd?
    list(FIND _CD_FOR livecd __cd)
    if(NOT __cd EQUAL -1)
        #manage dependency
        if(_CD_TARGET)
            add_dependencies(livecd ${_CD_TARGET})
        endif()
        foreach(item ${_CD_FILE})
            file(APPEND ${REACTOS_BINARY_DIR}/boot/livecd.cmake "file(COPY \"${item}\" DESTINATION \"\${CD_DIR}/${_CD_DESTINATION}\")\n")
        endforeach()
        if(_CD_NAME_ON_CD)
            get_filename_component(__file ${_CD_FILE} NAME)
            #rename it in the cd tree
            file(APPEND ${REACTOS_BINARY_DIR}/boot/livecd.cmake "file(RENAME \${CD_DIR}/${_CD_DESTINATION}/${__file} \${CD_DIR}/${_CD_DESTINATION}/${_CD_NAME_ON_CD})\n")
        endif()
    endif() #end livecd

    #do we add it to regtest?
    list(FIND _CD_FOR regtest __cd)
    if(NOT __cd EQUAL -1)
        #whether or not we should put it in reactos.cab or directly on cd
        if(_CD_NO_CAB)
            #directly on cd
            foreach(item ${_CD_FILE})
                file(APPEND ${REACTOS_BINARY_DIR}/boot/bootcdregtest.cmake "file(COPY \"${item}\" DESTINATION \"\${CD_DIR}/${_CD_DESTINATION}\")\n")
            endforeach()
            if(_CD_NAME_ON_CD)
                get_filename_component(__file ${_CD_FILE} NAME)
                #rename it in the cd tree
                file(APPEND ${REACTOS_BINARY_DIR}/boot/bootcdregtest.cmake "file(RENAME \${CD_DIR}/${_CD_DESTINATION}/${__file} \${CD_DIR}/${_CD_DESTINATION}/${_CD_NAME_ON_CD})\n")
            endif()
            if(_CD_TARGET)
                #manage dependency
                add_dependencies(bootcdregtest ${_CD_TARGET})
            endif()
        else()
            #add it in reactos.cab
            #dir_to_num(${_CD_DESTINATION} _num)
            #file(APPEND ${REACTOS_BINARY_DIR}/boot/bootdata/packages/reactos.dff.dyn "${_CD_FILE} ${_num}\n")
            #if(_CD_TARGET)
            #    #manage dependency
            #    add_dependencies(reactos_cab ${_CD_TARGET})
            #endif()
        endif()
    endif() #end bootcd
endfunction()

# Create module_clean targets
function(add_clean_target target)
    if(CMAKE_GENERATOR MATCHES "Unix Makefiles" OR CMAKE_GENERATOR MATCHES "MinGW Makefiles")
        set(CLEAN_COMMAND make clean)
    elseif(CMAKE_GENERATOR MATCHES "NMake Makefiles")
        set(CLEAN_COMMAND nmake clean)
    endif()
    add_custom_target(${target}_clean
        COMMAND ${CLEAN_COMMAND}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Cleaning ${target}")
endfunction()

if(NOT MSVC_IDE)
    function(add_library name)
        _add_library(${name} ${ARGN})
        add_clean_target(${name})
    endfunction()

    function(add_executable name)
        _add_executable(${name} ${ARGN})
        add_clean_target(${name})
    endfunction()
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES Windows)
    macro(to_win_path _cmake_path _native_path)
        string(REPLACE "/" "\\" ${_native_path} "${_cmake_path}")
    endmacro()

    macro(concatenate_files _file1 _file2 _output)
        to_win_path("${_file1}" _real_file1)
        to_win_path("${_file2}" _real_file2)
        to_win_path("${_output}" _real_output)
        add_custom_command(
            OUTPUT ${_output}
            COMMAND cmd.exe /C "copy /Y /B ${_real_file1} + ${_real_file2} ${_real_output} > nul"
            DEPENDS ${_file1}
            DEPENDS ${_file2})
    endmacro()
else()
    macro(concatenate_files _file1 _file2 _output)
        add_custom_command(
            OUTPUT ${_output}
            COMMAND cat ${_file1} ${_file2} > ${_output}
            DEPENDS ${_file1}
            DEPENDS ${_file2})
    endmacro()
endif()

macro(add_importlibs MODULE)
    add_dependency_node(${MODULE})
    foreach(LIB ${ARGN})
        if ("${LIB}" MATCHES "msvcrt")
            target_link_libraries(${MODULE} msvcrtex)
        endif()
        target_link_libraries(${MODULE} ${CMAKE_BINARY_DIR}/importlibs/lib${LIB}${CMAKE_STATIC_LIBRARY_SUFFIX})
        add_dependencies(${MODULE} lib${LIB})
        add_dependency_edge(${MODULE} ${LIB})
    endforeach()
endmacro()
