
function(add_dependency_node _node)
    if(GENERATE_DEPENDENCY_GRAPH)
        get_target_property(_type ${_node} TYPE)
        if(_type MATCHES SHARED_LIBRARY|MODULE_LIBRARY OR ${_node} MATCHES ntoskrnl)
            file(APPEND ${REACTOS_BINARY_DIR}/dependencies.graphml "    <node id=\"${_node}\"/>\n")
        endif()
     endif()
endfunction()

function(add_dependency_edge _source _target)
    if(GENERATE_DEPENDENCY_GRAPH)
        get_target_property(_type ${_source} TYPE)
        if(_type MATCHES SHARED_LIBRARY|MODULE_LIBRARY)
            file(APPEND ${REACTOS_BINARY_DIR}/dependencies.graphml "    <edge source=\"${_source}\" target=\"${_target}\"/>\n")
        endif()
    endif()
endfunction()

function(add_dependency_header)
    if(GENERATE_DEPENDENCY_GRAPH)
        file(WRITE ${REACTOS_BINARY_DIR}/dependencies.graphml "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<graphml>\n  <graph id=\"ReactOS dependencies\" edgedefault=\"directed\">\n")
    endif()
endfunction()

function(add_dependency_footer)
    if(GENERATE_DEPENDENCY_GRAPH)
        add_dependency_node(ntdll)
        file(APPEND ${REACTOS_BINARY_DIR}/dependencies.graphml "  </graph>\n</graphml>\n")
    endif()
endfunction()

function(add_message_headers _type)
    if(${_type} STREQUAL UNICODE)
        set(_flag "-U")
    else()
        set(_flag "-A")
    endif()
    foreach(_file ${ARGN})
        get_filename_component(_file_name ${_file} NAME_WE)
        set(_converted_file ${CMAKE_CURRENT_BINARY_DIR}/${_file}) ## ${_file_name}.mc
        set(_source_file ${CMAKE_CURRENT_SOURCE_DIR}/${_file})    ## ${_file_name}.mc
        add_custom_command(
            OUTPUT "${_converted_file}"
            COMMAND native-utf16le "${_source_file}" "${_converted_file}" nobom
            DEPENDS native-utf16le "${_source_file}")
        macro_mc(${_flag} ${_converted_file})
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_file_name}.h ${CMAKE_CURRENT_BINARY_DIR}/${_file_name}.rc
            COMMAND ${COMMAND_MC}
            DEPENDS "${_converted_file}")
        set_source_files_properties(
            ${CMAKE_CURRENT_BINARY_DIR}/${_file_name}.h ${CMAKE_CURRENT_BINARY_DIR}/${_file_name}.rc
            PROPERTIES GENERATED TRUE)
        add_custom_target(${_file_name} ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_file_name}.h ${CMAKE_CURRENT_BINARY_DIR}/${_file_name}.rc)
    endforeach()
endfunction()

function(add_link)
    cmake_parse_arguments(_LINK "MINIMIZE" "NAME;PATH;CMD_LINE_ARGS;ICON;GUID" "" ${ARGN})
    if(NOT _LINK_NAME OR NOT _LINK_PATH)
        message(FATAL_ERROR "You must provide name and path")
    endif()

    if(_LINK_CMD_LINE_ARGS)
        set(_LINK_CMD_LINE_ARGS -c ${_LINK_CMD_LINE_ARGS})
    endif()

    if(_LINK_ICON)
        set(_LINK_ICON -i ${_LINK_ICON})
    endif()

    if(_LINK_GUID)
        set(_LINK_GUID -g ${_LINK_GUID})
    endif()

    if(_LINK_MINIMIZE)
        set(_LINK_MINIMIZE "-m")
    endif()

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_LINK_NAME}.lnk
        COMMAND native-mkshelllink -o ${CMAKE_CURRENT_BINARY_DIR}/${_LINK_NAME}.lnk ${_LINK_CMD_LINE_ARGS} ${_LINK_ICON} ${_LINK_GUID} ${_LINK_MINIMIZE} ${_LINK_PATH}
        DEPENDS native-mkshelllink)
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${_LINK_NAME}.lnk PROPERTIES GENERATED TRUE)
endfunction()

#
# WARNING!
# Please keep the numbering in this list in sync with
# boot/bootdata/packages/reactos.dff.in
#
macro(dir_to_num dir var)
    if(${dir} STREQUAL reactos)
        set(${var} 1)
    elseif(${dir} STREQUAL reactos/system32)
        set(${var} 2)
    elseif(${dir} STREQUAL reactos/system32/drivers)
        set(${var} 3)
    elseif(${dir} STREQUAL reactos/Fonts)
        set(${var} 4)
    elseif(${dir} STREQUAL reactos/system32/drivers/etc)
        set(${var} 5)
    elseif(${dir} STREQUAL reactos/inf)
        set(${var} 6)
    elseif(${dir} STREQUAL reactos/bin)
        set(${var} 7)
    elseif(${dir} STREQUAL reactos/bin/testdata)
        set(${var} 8)
    elseif(${dir} STREQUAL reactos/media)
        set(${var} 9)
    elseif(${dir} STREQUAL reactos/Microsoft.NET)
        set(${var} 10)
    elseif(${dir} STREQUAL reactos/Microsoft.NET/Framework)
        set(${var} 11)
    elseif(${dir} STREQUAL reactos/Microsoft.NET/Framework/v1.0.3705)
        set(${var} 12)
    elseif(${dir} STREQUAL reactos/Microsoft.NET/Framework/v1.1.4322)
        set(${var} 13)
    elseif(${dir} STREQUAL reactos/Microsoft.NET/Framework/v2.0.50727)
        set(${var} 14)
    elseif(${dir} STREQUAL reactos/Resources)
        set(${var} 15)
    elseif(${dir} STREQUAL reactos/Resources/Themes)
        set(${var} 16)
    elseif(${dir} STREQUAL reactos/system32/wbem)
        set(${var} 17)
    elseif(${dir} STREQUAL reactos/Resources/Themes/Lautus)
        set(${var} 18)
    elseif(${dir} STREQUAL reactos/Help)
        set(${var} 19)
    elseif(${dir} STREQUAL reactos/Config)
        set(${var} 20)
    elseif(${dir} STREQUAL reactos/Cursors)
        set(${var} 21)
    elseif(${dir} STREQUAL reactos/system32/ShellExt)
        set(${var} 22)
    elseif(${dir} STREQUAL reactos/Temp)
        set(${var} 23)
    elseif(${dir} STREQUAL reactos/system32/spool)
        set(${var} 24)
    elseif(${dir} STREQUAL reactos/system32/spool/drivers)
        set(${var} 25)
    elseif(${dir} STREQUAL reactos/system32/spool/drivers/color)
        set(${var} 26)
    elseif(${dir} STREQUAL reactos/system32/spool/drivers/w32x86)
        set(${var} 27)
    elseif(${dir} STREQUAL reactos/system32/spool/drivers/w32x86/3)
        set(${var} 28)
    elseif(${dir} STREQUAL reactos/system32/spool/prtprocs)
        set(${var} 29)
    elseif(${dir} STREQUAL reactos/system32/spool/prtprocs/w32x86)
        set(${var} 30)
    elseif(${dir} STREQUAL reactos/system32/spool/PRINTERS)
        set(${var} 31)
    elseif(${dir} STREQUAL reactos/system32/wbem/Repository)
        set(${var} 32)
    elseif(${dir} STREQUAL reactos/system32/wbem/Repository/FS)
        set(${var} 33)
    elseif(${dir} STREQUAL reactos/system32/wbem/mof/good)
        set(${var} 34)
    elseif(${dir} STREQUAL reactos/system32/wbem/mof/bad)
        set(${var} 35)
    elseif(${dir} STREQUAL reactos/system32/wbem/AdStatus)
        set(${var} 36)
    elseif(${dir} STREQUAL reactos/system32/wbem/xml)
        set(${var} 37)
    elseif(${dir} STREQUAL reactos/system32/wbem/Logs)
        set(${var} 38)
    elseif(${dir} STREQUAL reactos/system32/wbem/AutoRecover)
        set(${var} 39)
    elseif(${dir} STREQUAL reactos/system32/wbem/snmp)
        set(${var} 40)
    elseif(${dir} STREQUAL reactos/system32/wbem/Performance)
        set(${var} 41)
    elseif(${dir} STREQUAL reactos/twain_32)
        set(${var} 42)
    elseif(${dir} STREQUAL reactos/repair)
        set(${var} 43)
    elseif(${dir} STREQUAL reactos/Web)
        set(${var} 44)
    elseif(${dir} STREQUAL reactos/Web/Wallpaper)
        set(${var} 45)
    elseif(${dir} STREQUAL reactos/Prefetch)
        set(${var} 46)
    elseif(${dir} STREQUAL reactos/security)
        set(${var} 47)
    elseif(${dir} STREQUAL reactos/security/Database)
        set(${var} 48)
    elseif(${dir} STREQUAL reactos/security/logs)
        set(${var} 49)
    elseif(${dir} STREQUAL reactos/security/templates)
        set(${var} 50)
    elseif(${dir} STREQUAL reactos/system32/CatRoot)
        set(${var} 51)
    elseif(${dir} STREQUAL reactos/system32/CatRoot2)
        set(${var} 52)
    elseif(${dir} STREQUAL reactos/AppPatch)
        set(${var} 53)
    elseif(${dir} STREQUAL reactos/winsxs)
        set(${var} 54)
    elseif(${dir} STREQUAL reactos/winsxs/manifests)
        set(${var} 55)
    elseif(${dir} STREQUAL reactos/winsxs/x86_microsoft.windows.common-controls_6595b64144ccf1df_5.82.2600.2982_none_deadbeef)
        set(${var} 56)
    elseif(${dir} STREQUAL reactos/winsxs/x86_microsoft.windows.common-controls_6595b64144ccf1df_6.0.2600.2982_none_deadbeef)
        set(${var} 57)
    elseif(${dir} STREQUAL reactos/winsxs/x86_microsoft.windows.gdiplus_6595b64144ccf1df_1.1.7601.23038_none_deadbeef)
        set(${var} 58)
    elseif(${dir} STREQUAL reactos/winsxs/x86_reactos.apisets_6595b64144ccf1df_1.0.0.0_none_deadbeef)
        set(${var} 59)
    elseif(${dir} STREQUAL reactos/bin/suppl)
        set(${var} 60)
    elseif(${dir} STREQUAL reactos/winsxs/x86_microsoft.windows.gdiplus_6595b64144ccf1df_1.0.14393.0_none_deadbeef)
        set(${var} 61)
    elseif(${dir} STREQUAL reactos/Resources/Themes/Modern)
        set(${var} 62)
    elseif(${dir} STREQUAL reactos/3rdParty)
        set(${var} 63)
    elseif(${dir} STREQUAL reactos/Resources/Themes/Lunar)
        set(${var} 64)
    elseif(${dir} STREQUAL reactos/Resources/Themes/Mizu)
        set(${var} 65)
    elseif(${dir} STREQUAL reactos/system32/spool/prtprocs/x64)
        set(${var} 66)
    elseif(${dir} STREQUAL reactos/winsxs/amd64_microsoft.windows.common-controls_6595b64144ccf1df_5.82.2600.2982_none_deadbeef)
        set(${var} 67)
    elseif(${dir} STREQUAL reactos/winsxs/amd64_microsoft.windows.common-controls_6595b64144ccf1df_6.0.2600.2982_none_deadbeef)
        set(${var} 68)
    elseif(${dir} STREQUAL reactos/winsxs/amd64_microsoft.windows.gdiplus_6595b64144ccf1df_1.1.7601.23038_none_deadbeef)
        set(${var} 69)
    elseif(${dir} STREQUAL reactos/winsxs/amd64_reactos.apisets_6595b64144ccf1df_1.0.0.0_none_deadbeef)
        set(${var} 70)
    elseif(${dir} STREQUAL reactos/winsxs/amd64_microsoft.windows.gdiplus_6595b64144ccf1df_1.0.14393.0_none_deadbeef)
        set(${var} 71)

    elseif(${dir} STREQUAL reactos/winsxs/arm_microsoft.windows.common-controls_6595b64144ccf1df_5.82.2600.2982_none_deadbeef)
        set(${var} 72)
    elseif(${dir} STREQUAL reactos/winsxs/arm_microsoft.windows.common-controls_6595b64144ccf1df_6.0.2600.2982_none_deadbeef)
        set(${var} 73)
    elseif(${dir} STREQUAL reactos/winsxs/arm_microsoft.windows.gdiplus_6595b64144ccf1df_1.1.7601.23038_none_deadbeef)
        set(${var} 74)
    elseif(${dir} STREQUAL reactos/winsxs/arm_reactos.apisets_6595b64144ccf1df_1.0.0.0_none_deadbeef)
        set(${var} 75)
    elseif(${dir} STREQUAL reactos/winsxs/arm_microsoft.windows.gdiplus_6595b64144ccf1df_1.0.14393.0_none_deadbeef)
        set(${var} 76)

    elseif(${dir} STREQUAL reactos/winsxs/arm64_microsoft.windows.common-controls_6595b64144ccf1df_5.82.2600.2982_none_deadbeef)
        set(${var} 77)
    elseif(${dir} STREQUAL reactos/winsxs/arm64_microsoft.windows.common-controls_6595b64144ccf1df_6.0.2600.2982_none_deadbeef)
        set(${var} 78)
    elseif(${dir} STREQUAL reactos/winsxs/arm64_microsoft.windows.gdiplus_6595b64144ccf1df_1.1.7601.23038_none_deadbeef)
        set(${var} 79)
    elseif(${dir} STREQUAL reactos/winsxs/arm64_reactos.apisets_6595b64144ccf1df_1.0.0.0_none_deadbeef)
        set(${var} 80)
    elseif(${dir} STREQUAL reactos/winsxs/arm64_microsoft.windows.gdiplus_6595b64144ccf1df_1.0.14393.0_none_deadbeef)
        set(${var} 81)


    else()
        message(FATAL_ERROR "Wrong destination: ${dir}")
    endif()
endmacro()

function(add_cd_file)
    cmake_parse_arguments(_CD "NO_CAB;NOT_IN_HYBRIDCD" "DESTINATION;NAME_ON_CD;TARGET" "FILE;FOR" ${ARGN})
    if(NOT (_CD_TARGET OR _CD_FILE))
        message(FATAL_ERROR "You must provide a target or a file to install!")
    endif()

    if(NOT _CD_DESTINATION)
        message(FATAL_ERROR "You must provide a destination")
    elseif(${_CD_DESTINATION} STREQUAL root)
        set(_CD_DESTINATION "")
    endif()

    if(NOT _CD_FOR)
        message(FATAL_ERROR "You must provide a cd name (or \"all\" for all of them) to install the file on!")
    endif()

    # get file if we need to
    if(NOT _CD_FILE)
        set(_CD_FILE "$<TARGET_FILE:${_CD_TARGET}>")
        if(NOT _CD_NAME_ON_CD)
            set(_CD_NAME_ON_CD "$<TARGET_FILE_NAME:${_CD_TARGET}>")
        endif()
    endif()

    # do we add it to all CDs?
    list(FIND _CD_FOR all __cd)
    if(NOT __cd EQUAL -1)
        list(REMOVE_AT _CD_FOR __cd)
        list(INSERT _CD_FOR __cd "bootcd;livecd;regtest")
    endif()

    # do we add it to bootcd?
    list(FIND _CD_FOR bootcd __cd)
    if(NOT __cd EQUAL -1)
        # whether or not we should put it in reactos.cab or directly on cd
        if(_CD_NO_CAB)
            # directly on cd
            foreach(item ${_CD_FILE})
                if(_CD_NAME_ON_CD)
                    # rename it in the cd tree
                    set(__file ${_CD_NAME_ON_CD})
                else()
                    get_filename_component(__file ${item} NAME)
                endif()
                set_property(GLOBAL APPEND PROPERTY BOOTCD_FILE_LIST "${_CD_DESTINATION}/${__file}=${item}")
                # add it also into the hybridcd if not specified otherwise
                if(NOT _CD_NOT_IN_HYBRIDCD)
                    set_property(GLOBAL APPEND PROPERTY HYBRIDCD_FILE_LIST "bootcd/${_CD_DESTINATION}/${__file}=${item}")
                endif()
            endforeach()
            # manage dependency
            if(_CD_TARGET)
                add_dependencies(bootcd ${_CD_TARGET} registry_inf)
            endif()
        else()
            dir_to_num(${_CD_DESTINATION} _num)
            foreach(item ${_CD_FILE})
                # add it in reactos.cab
                file(APPEND ${REACTOS_BINARY_DIR}/boot/bootdata/packages/reactos.dff.cmake "\"${item}\" ${_num}\n")

                # manage dependency - file level
                set_property(GLOBAL APPEND PROPERTY REACTOS_CAB_DEPENDS ${item})
            endforeach()

            # manage dependency - target level
            if(_CD_TARGET)
                add_dependencies(reactos_cab_inf ${_CD_TARGET})
            endif()
        endif()
    endif() #end bootcd

    # do we add it to livecd?
    list(FIND _CD_FOR livecd __cd)
    if(NOT __cd EQUAL -1)
        # manage dependency
        if(_CD_TARGET)
            add_dependencies(livecd ${_CD_TARGET} registry_inf)
        endif()
        foreach(item ${_CD_FILE})
            if(_CD_NAME_ON_CD)
                # rename it in the cd tree
                set(__file ${_CD_NAME_ON_CD})
            else()
                get_filename_component(__file ${item} NAME)
            endif()
            set_property(GLOBAL APPEND PROPERTY LIVECD_FILE_LIST "${_CD_DESTINATION}/${__file}=${item}")
            # add it also into the hybridcd if not specified otherwise
            if(NOT _CD_NOT_IN_HYBRIDCD)
                set_property(GLOBAL APPEND PROPERTY HYBRIDCD_FILE_LIST "livecd/${_CD_DESTINATION}/${__file}=${item}")
            endif()
        endforeach()
    endif() #end livecd

    # do we need also to add it to hybridcd?
    list(FIND _CD_FOR hybridcd __cd)
    if(NOT __cd EQUAL -1)
        # manage dependency
        if(_CD_TARGET)
            add_dependencies(hybridcd ${_CD_TARGET})
        endif()
        foreach(item ${_CD_FILE})
            if(_CD_NAME_ON_CD)
                # rename it in the cd tree
                set(__file ${_CD_NAME_ON_CD})
            else()
                get_filename_component(__file ${item} NAME)
            endif()
            set_property(GLOBAL APPEND PROPERTY HYBRIDCD_FILE_LIST "${_CD_DESTINATION}/${__file}=${item}")
        endforeach()
    endif() #end hybridcd

    # do we add it to regtest?
    list(FIND _CD_FOR regtest __cd)
    if(NOT __cd EQUAL -1)
        # whether or not we should put it in reactos.cab or directly on cd
        if(_CD_NO_CAB)
            # directly on cd
            foreach(item ${_CD_FILE})
                if(_CD_NAME_ON_CD)
                    # rename it in the cd tree
                    set(__file ${_CD_NAME_ON_CD})
                else()
                    get_filename_component(__file ${item} NAME)
                endif()
                set_property(GLOBAL APPEND PROPERTY BOOTCDREGTEST_FILE_LIST "${_CD_DESTINATION}/${__file}=${item}")
            endforeach()
            # manage dependency
            if(_CD_TARGET)
                add_dependencies(bootcdregtest ${_CD_TARGET} registry_inf)
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

function(create_iso_lists)
    # generate reactos.cab before anything else
    get_property(_filelist GLOBAL PROPERTY REACTOS_CAB_DEPENDS)

    # begin with reactos.inf. We want this command to be always executed, so we pretend it generates another file although it will never do.
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/reactos.inf ${CMAKE_CURRENT_BINARY_DIR}/__some_non_existent_file
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${REACTOS_BINARY_DIR}/boot/bootdata/packages/reactos.inf ${CMAKE_CURRENT_BINARY_DIR}/reactos.inf
        DEPENDS ${REACTOS_BINARY_DIR}/boot/bootdata/packages/reactos.inf reactos_cab_inf)

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/reactos.cab
        COMMAND native-cabman -C ${REACTOS_BINARY_DIR}/boot/bootdata/packages/reactos.dff -RC ${CMAKE_CURRENT_BINARY_DIR}/reactos.inf -N -P ${REACTOS_SOURCE_DIR}
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/reactos.inf native-cabman ${_filelist})

    add_custom_target(reactos_cab DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/reactos.cab)
    add_dependencies(reactos_cab reactos_cab_inf)

    add_cd_file(
        TARGET reactos_cab
        FILE ${CMAKE_CURRENT_BINARY_DIR}/reactos.cab
        DESTINATION reactos
        NO_CAB FOR bootcd regtest)

    add_cd_file(
        FILE ${CMAKE_CURRENT_BINARY_DIR}/livecd.iso
        DESTINATION livecd
        FOR hybridcd)

    get_property(_filelist GLOBAL PROPERTY BOOTCD_FILE_LIST)
    string(REPLACE ";" "\n" _filelist "${_filelist}")
    file(APPEND ${REACTOS_BINARY_DIR}/boot/bootcd.cmake.lst "${_filelist}")
    unset(_filelist)
    file(GENERATE
         OUTPUT ${REACTOS_BINARY_DIR}/boot/bootcd.$<CONFIG>.lst
         INPUT ${REACTOS_BINARY_DIR}/boot/bootcd.cmake.lst)

    get_property(_filelist GLOBAL PROPERTY LIVECD_FILE_LIST)
    string(REPLACE ";" "\n" _filelist "${_filelist}")
    file(APPEND ${REACTOS_BINARY_DIR}/boot/livecd.cmake.lst "${_filelist}")
    unset(_filelist)
    file(GENERATE
         OUTPUT ${REACTOS_BINARY_DIR}/boot/livecd.$<CONFIG>.lst
         INPUT ${REACTOS_BINARY_DIR}/boot/livecd.cmake.lst)

    get_property(_filelist GLOBAL PROPERTY HYBRIDCD_FILE_LIST)
    string(REPLACE ";" "\n" _filelist "${_filelist}")
    file(APPEND ${REACTOS_BINARY_DIR}/boot/hybridcd.cmake.lst "${_filelist}")
    unset(_filelist)
    file(GENERATE
         OUTPUT ${REACTOS_BINARY_DIR}/boot/hybridcd.$<CONFIG>.lst
         INPUT ${REACTOS_BINARY_DIR}/boot/hybridcd.cmake.lst)

    get_property(_filelist GLOBAL PROPERTY BOOTCDREGTEST_FILE_LIST)
    string(REPLACE ";" "\n" _filelist "${_filelist}")
    file(APPEND ${REACTOS_BINARY_DIR}/boot/bootcdregtest.cmake.lst "${_filelist}")
    unset(_filelist)
    file(GENERATE
         OUTPUT ${REACTOS_BINARY_DIR}/boot/bootcdregtest.$<CONFIG>.lst
         INPUT ${REACTOS_BINARY_DIR}/boot/bootcdregtest.cmake.lst)
endfunction()

# Create module_clean targets
function(add_clean_target _target)
    set(_clean_working_directory ${CMAKE_CURRENT_BINARY_DIR})
    if(CMAKE_GENERATOR STREQUAL "Unix Makefiles" OR CMAKE_GENERATOR STREQUAL "MinGW Makefiles")
        set(_clean_command make clean)
    elseif(CMAKE_GENERATOR STREQUAL "NMake Makefiles")
        set(_clean_command nmake /nologo clean)
    elseif(CMAKE_GENERATOR STREQUAL "Ninja")
        set(_clean_command ninja -t clean ${_target})
        set(_clean_working_directory ${REACTOS_BINARY_DIR})
    endif()
    add_custom_target(${_target}_clean
        COMMAND ${_clean_command}
        WORKING_DIRECTORY ${_clean_working_directory}
        COMMENT "Cleaning ${_target}")
endfunction()

if(NOT MSVC_IDE)
    function(add_library name)
        _add_library(${name} ${ARGN})
        add_clean_target(${name})
        # cmake adds a module_EXPORTS define when compiling a module or a shared library. We don't use that.
        get_target_property(_type ${name} TYPE)
        if(_type MATCHES SHARED_LIBRARY|MODULE_LIBRARY)
            set_target_properties(${name} PROPERTIES DEFINE_SYMBOL "")
        endif()
    endfunction()

    function(add_executable name)
        _add_executable(${name} ${ARGN})
        add_clean_target(${name})
    endfunction()
elseif(USE_FOLDER_STRUCTURE)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    string(LENGTH ${CMAKE_SOURCE_DIR} CMAKE_SOURCE_DIR_LENGTH)

    function(add_custom_target name)
        _add_custom_target(${name} ${ARGN})
        string(SUBSTRING ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR_LENGTH} -1 CMAKE_CURRENT_SOURCE_DIR_RELATIVE)
        set_property(TARGET "${name}" PROPERTY FOLDER "${CMAKE_CURRENT_SOURCE_DIR_RELATIVE}")
    endfunction()

    function(add_library name)
        _add_library(${name} ${ARGN})
        get_target_property(_type ${name} TYPE)
        if (NOT _type STREQUAL "INTERFACE_LIBRARY")
            get_target_property(_target_excluded ${name} EXCLUDE_FROM_ALL)
            if(_target_excluded AND ${name} MATCHES "^lib.*")
                set_property(TARGET "${name}" PROPERTY FOLDER "Importlibs")
            else()
                string(SUBSTRING ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR_LENGTH} -1 CMAKE_CURRENT_SOURCE_DIR_RELATIVE)
                set_property(TARGET "${name}" PROPERTY FOLDER "${CMAKE_CURRENT_SOURCE_DIR_RELATIVE}")
            endif()
        endif()
        # cmake adds a module_EXPORTS define when compiling a module or a shared library. We don't use that.
        if(_type MATCHES SHARED_LIBRARY|MODULE_LIBRARY)
            set_target_properties(${name} PROPERTIES DEFINE_SYMBOL "")
        endif()
    endfunction()

    function(add_executable name)
        _add_executable(${name} ${ARGN})
        string(SUBSTRING ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR_LENGTH} -1 CMAKE_CURRENT_SOURCE_DIR_RELATIVE)
        set_property(TARGET "${name}" PROPERTY FOLDER "${CMAKE_CURRENT_SOURCE_DIR_RELATIVE}")
    endfunction()
else()
    function(add_library name)
        _add_library(${name} ${ARGN})
        # cmake adds a module_EXPORTS define when compiling a module or a shared library. We don't use that.
        get_target_property(_type ${name} TYPE)
        if(_type MATCHES SHARED_LIBRARY|MODULE_LIBRARY)
            set_target_properties(${name} PROPERTIES DEFINE_SYMBOL "")
        endif()
    endfunction()
endif()

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    function(concatenate_files _output _file1)
        file(TO_NATIVE_PATH "${_output}" _real_output)
        file(TO_NATIVE_PATH "${_file1}" _file_list)
        foreach(_file ${ARGN})
            file(TO_NATIVE_PATH "${_file}" _real_file)
            set(_file_list "${_file_list} + ${_real_file}")
        endforeach()
        add_custom_command(
            OUTPUT ${_output}
            COMMAND cmd.exe /C "copy /Y /B ${_file_list} ${_real_output} > nul"
            DEPENDS ${_file1} ${ARGN})
    endfunction()
else()
    macro(concatenate_files _output)
        add_custom_command(
            OUTPUT ${_output}
            COMMAND cat ${ARGN} > ${_output}
            DEPENDS ${ARGN})
    endmacro()
endif()

function(add_importlibs _module)
    add_dependency_node(${_module})
    foreach(LIB ${ARGN})
        target_link_libraries(${_module} lib${LIB})
        add_dependency_edge(${_module} ${LIB})
    endforeach()
endfunction()

# Some helper lists
list(APPEND VALID_MODULE_TYPES kernel kerneldll kernelmodedriver wdmdriver nativecui nativedll win32cui win32gui win32dll win32ocx cpl module)
list(APPEND KERNEL_MODULE_TYPES kernel kerneldll kernelmodedriver wdmdriver)
list(APPEND NATIVE_MODULE_TYPES kernel kerneldll kernelmodedriver wdmdriver nativecui nativedll)

function(set_module_type MODULE TYPE)
    cmake_parse_arguments(__module "UNICODE" "IMAGEBASE" "ENTRYPOINT" ${ARGN})

    if(__module_UNPARSED_ARGUMENTS)
        message(STATUS "set_module_type : unparsed arguments ${__module_UNPARSED_ARGUMENTS}, module : ${MODULE}")
    endif()

    # Check this is a type that we know
    if (NOT TYPE IN_LIST VALID_MODULE_TYPES)
        message(FATAL_ERROR "Unknown type ${TYPE} for module ${MODULE}")
    endif()

    # Set our target property
    set_target_properties(${MODULE} PROPERTIES REACTOS_MODULE_TYPE ${TYPE})

    # Add the module to the module group list, if it is defined
    if(DEFINED CURRENT_MODULE_GROUP)
        set_property(GLOBAL APPEND PROPERTY ${CURRENT_MODULE_GROUP}_MODULE_LIST "${MODULE}")
    endif()

    # Set subsystem.
    if(TYPE IN_LIST NATIVE_MODULE_TYPES)
        set_subsystem(${MODULE} native)
    elseif(${TYPE} STREQUAL win32cui)
        set_subsystem(${MODULE} console)
    elseif(${TYPE} STREQUAL win32gui)
        set_subsystem(${MODULE} windows)
    endif()

    # Set unicode definitions
    if(__module_UNICODE)
        target_compile_definitions(${MODULE} PRIVATE UNICODE _UNICODE)
    endif()

    # Set entry point
    if(__module_ENTRYPOINT OR (__module_ENTRYPOINT STREQUAL "0"))
        set_entrypoint(${MODULE} ${__module_ENTRYPOINT})
    elseif(${TYPE} STREQUAL nativecui)
        set_entrypoint(${MODULE} NtProcessStartup 4)
    elseif(${TYPE} STREQUAL win32cui)
        if(__module_UNICODE)
            set_entrypoint(${MODULE} wmainCRTStartup)
        else()
            set_entrypoint(${MODULE} mainCRTStartup)
        endif()
    elseif(${TYPE} STREQUAL win32gui)
        if(__module_UNICODE)
            set_entrypoint(${MODULE} wWinMainCRTStartup)
        else()
            set_entrypoint(${MODULE} WinMainCRTStartup)
        endif()
    elseif((${TYPE} STREQUAL win32dll) OR (${TYPE} STREQUAL win32ocx)
            OR (${TYPE} STREQUAL cpl))
        set_entrypoint(${MODULE} DllMainCRTStartup 12)
    elseif((${TYPE} STREQUAL kernelmodedriver) OR (${TYPE} STREQUAL wdmdriver))
        set_entrypoint(${MODULE} DriverEntry 8)
    elseif(${TYPE} STREQUAL nativedll)
        set_entrypoint(${MODULE} DllMain 12)
    elseif(TYPE STREQUAL kernel)
        set_entrypoint(${MODULE} KiSystemStartup 4)
    elseif(${TYPE} STREQUAL module)
        set_entrypoint(${MODULE} 0)
    endif()

    # Set base address
    if(__module_IMAGEBASE)
        set_image_base(${MODULE} ${__module_IMAGEBASE})
    elseif(${TYPE} STREQUAL win32dll)
        if(DEFINED baseaddress_${MODULE})
            set_image_base(${MODULE} ${baseaddress_${MODULE}})
        else()
            message(STATUS "${MODULE} has no base address")
        endif()
    elseif(TYPE IN_LIST KERNEL_MODULE_TYPES)
        # special case for kernel
        if (TYPE STREQUAL kernel)
            set_image_base(${MODULE} 0x00400000)
        else()
            set_image_base(${MODULE} 0x00010000)
        endif()
    endif()

    # Now do some stuff which is specific to each type
    if(TYPE IN_LIST KERNEL_MODULE_TYPES)
        add_dependencies(${MODULE} bugcodes xdk)
        if((${TYPE} STREQUAL kernelmodedriver) OR (${TYPE} STREQUAL wdmdriver))
            set_target_properties(${MODULE} PROPERTIES SUFFIX ".sys")
        endif()
    endif()

    if(TYPE STREQUAL kernel)
        # Kernels are executables with exports
        set_property(TARGET ${MODULE} PROPERTY ENABLE_EXPORTS TRUE)
        set_target_properties(${MODULE} PROPERTIES DEFINE_SYMBOL "")
    endif()

    if(${TYPE} STREQUAL win32ocx)
        set_target_properties(${MODULE} PROPERTIES SUFFIX ".ocx")
    endif()

    if(${TYPE} STREQUAL cpl)
        set_target_properties(${MODULE} PROPERTIES SUFFIX ".cpl")
    endif()

    # Do compiler specific stuff
    set_module_type_toolchain(${MODULE} ${TYPE})
endfunction()

function(start_module_group __name)
    if(DEFINED CURRENT_MODULE_GROUP)
        message(FATAL_ERROR "CURRENT_MODULE_GROUP is already set ('${CURRENT_MODULE_GROUP}')")
    endif()
    set(CURRENT_MODULE_GROUP ${__name} PARENT_SCOPE)
endfunction()

function(end_module_group)
    get_property(__modulelist GLOBAL PROPERTY ${CURRENT_MODULE_GROUP}_MODULE_LIST)
    add_custom_target(${CURRENT_MODULE_GROUP})
    foreach(__module ${__modulelist})
        add_dependencies(${CURRENT_MODULE_GROUP} ${__module})
    endforeach()
    set(CURRENT_MODULE_GROUP PARENT_SCOPE)
endfunction()

function(preprocess_file __in __out)
    set(__arg ${__in})
    foreach(__def ${ARGN})
        list(APPEND __arg -D${__def})
    endforeach()
    if(MSVC)
        add_custom_command(OUTPUT ${_out}
            COMMAND ${CMAKE_C_COMPILER} /EP ${__arg}
            DEPENDS ${__in})
    else()
        add_custom_command(OUTPUT ${_out}
            COMMAND ${CMAKE_C_COMPILER} -E ${__arg}
            DEPENDS ${__in})
    endif()
endfunction()

function(get_includes OUTPUT_VAR)
    get_directory_property(_includes INCLUDE_DIRECTORIES)
    foreach(arg ${_includes})
        list(APPEND __tmp_var -I${arg})
    endforeach()
    set(${OUTPUT_VAR} ${__tmp_var} PARENT_SCOPE)
endfunction()

function(get_defines OUTPUT_VAR)
    get_directory_property(_defines COMPILE_DEFINITIONS)
    foreach(arg ${_defines})
        # Skip generator expressions
        if (NOT arg MATCHES [[^\$<.*>$]])
            list(APPEND __tmp_var -D${arg})
        endif()
    endforeach()
    set(${OUTPUT_VAR} ${__tmp_var} PARENT_SCOPE)
endfunction()

function(add_registry_inf)
    # Add to the inf files list
    foreach(_file ${ARGN})
        set(_source_file "${CMAKE_CURRENT_SOURCE_DIR}/${_file}")
        set_property(GLOBAL APPEND PROPERTY REGISTRY_INF_LIST ${_source_file})
    endforeach()
endfunction()

function(create_registry_hives)

    # Shortcut to the registry.inf file
    set(_registry_inf "${CMAKE_BINARY_DIR}/boot/bootdata/registry.inf")

    # Get the list of inf files
    get_property(_inf_files GLOBAL PROPERTY REGISTRY_INF_LIST)

    # Convert files to utf16le
    foreach(_file ${_inf_files})
        get_filename_component(_file_name ${_file} NAME_WE)
        file(RELATIVE_PATH _subdir ${CMAKE_SOURCE_DIR} ${_file})
        get_filename_component(_subdir ${_subdir}  DIRECTORY)
        set(_converted_file ${CMAKE_BINARY_DIR}/${_subdir}/${_file_name}_utf16.inf)
        add_custom_command(OUTPUT ${_converted_file}
                           COMMAND native-utf16le ${_file} ${_converted_file}
                           DEPENDS native-utf16le ${_file})
        list(APPEND _converted_files ${_converted_file})
    endforeach()

    # Concatenate all registry files to registry.inf
    concatenate_files(${_registry_inf} ${_converted_files})

    # Add registry.inf to bootcd
    add_custom_target(registry_inf DEPENDS ${_registry_inf})
    add_cd_file(TARGET registry_inf
                FILE ${_registry_inf}
                DESTINATION reactos
                NO_CAB
                FOR bootcd regtest)

    # BootCD setup system hive
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/boot/bootdata/SETUPREG.HIV
        COMMAND native-mkhive -h:SETUPREG -u -d:${CMAKE_BINARY_DIR}/boot/bootdata ${_registry_inf} ${CMAKE_SOURCE_DIR}/boot/bootdata/setupreg.inf
        DEPENDS native-mkhive ${_registry_inf})

    add_custom_target(bootcd_hives
        DEPENDS ${CMAKE_BINARY_DIR}/boot/bootdata/SETUPREG.HIV)

    add_cd_file(
        FILE ${CMAKE_BINARY_DIR}/boot/bootdata/SETUPREG.HIV
        TARGET bootcd_hives
        DESTINATION reactos
        NO_CAB
        FOR bootcd regtest)

    # LiveCD hives
    list(APPEND _livecd_inf_files
        ${_registry_inf}
        ${CMAKE_SOURCE_DIR}/boot/bootdata/livecd.inf
        ${CMAKE_SOURCE_DIR}/boot/bootdata/caroots.inf)
    if(SARCH STREQUAL "xbox")
        list(APPEND _livecd_inf_files
            ${CMAKE_SOURCE_DIR}/boot/bootdata/hiveinst_xbox.inf)
    elseif(SARCH STREQUAL "pc98")
        list(APPEND _livecd_inf_files
            ${CMAKE_SOURCE_DIR}/boot/bootdata/hiveinst_pc98.inf)
    else()
        list(APPEND _livecd_inf_files
            ${CMAKE_SOURCE_DIR}/boot/bootdata/hiveinst.inf)
    endif()

    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/boot/bootdata/system
               ${CMAKE_BINARY_DIR}/boot/bootdata/software
               ${CMAKE_BINARY_DIR}/boot/bootdata/default
               ${CMAKE_BINARY_DIR}/boot/bootdata/sam
               ${CMAKE_BINARY_DIR}/boot/bootdata/security
        COMMAND native-mkhive -h:SYSTEM,SOFTWARE,DEFAULT,SAM,SECURITY -d:${CMAKE_BINARY_DIR}/boot/bootdata ${_livecd_inf_files}
        DEPENDS native-mkhive ${_livecd_inf_files})

    add_custom_target(livecd_hives
        DEPENDS ${CMAKE_BINARY_DIR}/boot/bootdata/system
                ${CMAKE_BINARY_DIR}/boot/bootdata/software
                ${CMAKE_BINARY_DIR}/boot/bootdata/default
                ${CMAKE_BINARY_DIR}/boot/bootdata/sam
                ${CMAKE_BINARY_DIR}/boot/bootdata/security)

    add_cd_file(
        FILE ${CMAKE_BINARY_DIR}/boot/bootdata/system
             ${CMAKE_BINARY_DIR}/boot/bootdata/software
             ${CMAKE_BINARY_DIR}/boot/bootdata/default
             ${CMAKE_BINARY_DIR}/boot/bootdata/sam
             ${CMAKE_BINARY_DIR}/boot/bootdata/security
        TARGET livecd_hives
        DESTINATION reactos/system32/config
        FOR livecd)

    # BCD Hive
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/boot/bootdata/BCD
        COMMAND native-mkhive -h:BCD -u -d:${CMAKE_BINARY_DIR}/boot/bootdata ${CMAKE_BINARY_DIR}/boot/bootdata/hivebcd_utf16.inf
        DEPENDS native-mkhive ${CMAKE_BINARY_DIR}/boot/bootdata/hivebcd_utf16.inf)

    add_custom_target(bcd_hive
        DEPENDS ${CMAKE_BINARY_DIR}/boot/bootdata/BCD)

    add_cd_file(
        FILE ${CMAKE_BINARY_DIR}/boot/bootdata/BCD
        TARGET bcd_hive
        DESTINATION efi/boot
        NO_CAB
        FOR bootcd regtest livecd)

endfunction()

function(add_driver_inf _module)
    # Add to the inf files list
    foreach(_file ${ARGN})
        set(_converted_item ${CMAKE_CURRENT_BINARY_DIR}/${_file})
        set(_source_item ${CMAKE_CURRENT_SOURCE_DIR}/${_file})
        add_custom_command(OUTPUT "${_converted_item}"
                           COMMAND native-utf16le "${_source_item}" "${_converted_item}"
                           DEPENDS native-utf16le "${_source_item}")
        list(APPEND _converted_inf_files ${_converted_item})
    endforeach()

    add_custom_target(${_module}_inf_files DEPENDS ${_converted_inf_files})
    add_cd_file(FILE ${_converted_inf_files} TARGET ${_module}_inf_files DESTINATION reactos/inf FOR all)
endfunction()

if(KDBG)
    set(ROSSYM_LIB "rossym")
else()
    set(ROSSYM_LIB "")
endif()

function(add_rc_deps _target_rc)
    set_source_files_properties(${_target_rc} PROPERTIES OBJECT_DEPENDS "${ARGN}")
endfunction()

add_custom_target(rostests_install COMMAND ${CMAKE_COMMAND} -DCOMPONENT=rostests -P ${CMAKE_BINARY_DIR}/cmake_install.cmake)
function(add_rostests_file)
    cmake_parse_arguments(_ROSTESTS "" "RENAME;SUBDIR;TARGET" "FILE" ${ARGN})
    if(NOT (_ROSTESTS_TARGET OR _ROSTESTS_FILE))
        message(FATAL_ERROR "You must provide a target or a file to install!")
    endif()

    set(_ROSTESTS_NAME_ON_CD "${_ROSTESTS_RENAME}")
    if(NOT _ROSTESTS_FILE)
        set(_ROSTESTS_FILE "$<TARGET_FILE:${_ROSTESTS_TARGET}>")
        if(NOT _ROSTESTS_RENAME)
            set(_ROSTESTS_NAME_ON_CD "$<TARGET_FILE_NAME:${_ROSTESTS_TARGET}>")
        endif()
    else()
        if(NOT _ROSTESTS_RENAME)
            get_filename_component(_ROSTESTS_NAME_ON_CD ${_ROSTESTS_FILE} NAME)
        endif()
    endif()

    if(_ROSTESTS_SUBDIR)
        set(_ROSTESTS_SUBDIR "/${_ROSTESTS_SUBDIR}")
    endif()

    if(_ROSTESTS_TARGET)
        add_cd_file(TARGET ${_ROSTESTS_TARGET} FILE ${_ROSTESTS_FILE} DESTINATION "reactos/bin${_ROSTESTS_SUBDIR}" NAME_ON_CD ${_ROSTESTS_NAME_ON_CD} FOR all)
    else()
        add_cd_file(FILE ${_ROSTESTS_FILE} DESTINATION "reactos/bin${_ROSTESTS_SUBDIR}" NAME_ON_CD ${_ROSTESTS_NAME_ON_CD} FOR all)
    endif()

    if(DEFINED ENV{ROSTESTS_INSTALL})
        if(_ROSTESTS_RENAME)
            install(FILES ${_ROSTESTS_FILE} DESTINATION "$ENV{ROSTESTS_INSTALL}${_ROSTESTS_SUBDIR}" COMPONENT rostests RENAME ${_ROSTESTS_RENAME})
        else()
            install(FILES ${_ROSTESTS_FILE} DESTINATION "$ENV{ROSTESTS_INSTALL}${_ROSTESTS_SUBDIR}" COMPONENT rostests)
        endif()
    endif()
endfunction()

if(PCH)
    macro(add_pch _target _pch _skip_list)
        target_precompile_headers(${_target} PRIVATE ${_pch})
        set_source_files_properties(${_skip_list} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
    endmacro()
else()
    macro(add_pch _target _pch _skip_list)
    endmacro()
endif()

function(set_target_cpp_properties _target)
    cmake_parse_arguments(_CPP "WITH_EXCEPTIONS;WITH_RTTI" "" "" ${ARGN})

    if (_CPP_WITH_EXCEPTIONS)
        set_target_properties(${_target} PROPERTIES WITH_CXX_EXCEPTIONS TRUE)
    endif()

    if (_CPP_WITH_RTTI)
        set_target_properties(${_target} PROPERTIES WITH_CXX_RTTI TRUE)
    endif()
endfunction()
