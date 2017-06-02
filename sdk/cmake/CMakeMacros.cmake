
# set_cpp
#  Marks the current folder as containing C++ modules, additionally enabling
#  specific C++ language features as specified (all of these default to off):
#
#  WITH_RUNTIME
#   Links with the C++ runtime. Enable this for modules which use new/delete or
#   RTTI, but do not require STL. This is the right choice if you see undefined
#   references to operator new/delete, vector constructor/destructor iterator,
#   type_info::vtable, ...
#   Note: this only affects linking, so cannot be used for static libraries.
#  WITH_RTTI
#   Enables run-time type information. Enable this if the module uses typeid or
#   dynamic_cast. You will probably need to enable WITH_RUNTIME as well, if
#   you're not already using STL.
#  WITH_EXCEPTIONS
#   Enables C++ exception handling. Enable this if the module uses try/catch or
#   throw. You might also need this if you use a standard operator new (the one
#   without nothrow).
#  WITH_STL
#   Enables standard C++ headers and links to the Standard Template Library.
#   Use this for modules using anything from the std:: namespace, e.g. maps,
#   strings, vectors, etc.
#   Note: this affects both compiling (via include directories) and
#         linking (by adding STL). Implies WITH_RUNTIME.
#   FIXME: WITH_STL is currently also required for runtime headers such as
#          <new> and <exception>. This is not a big issue because in stl-less
#          environments you usually don't want those anyway; but we might want
#          to have modules like this in the future.
#
# Examples:
#  set_cpp()
#   Enables the C++ language, but will cause errors if any runtime or standard
#   library features are used. This should be the default for C++ in kernel
#   mode or otherwise restricted environments.
#   Note: this is required to get libgcc (for multiplication/division) linked
#         in for C++ modules, and to set the correct language for precompiled
#         header files, so it IS required even with no features specified.
#  set_cpp(WITH_RUNTIME)
#   Links with the C++ runtime, so that e.g. custom operator new implementations
#   can be used in a restricted environment. This is also required for linking
#   with libraries (such as ATL) which have RTTI enabled, even if the module in
#   question does not use WITH_RTTI.
#  set_cpp(WITH_RTTI WITH_EXCEPTIONS WITH_STL)
#   The full package. This will adjust compiler and linker so that all C++
#   features can be used.
macro(set_cpp)
    cmake_parse_arguments(__cppopts "WITH_RUNTIME;WITH_RTTI;WITH_EXCEPTIONS;WITH_STL" "" "" ${ARGN})
    if(__cppopts_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "set_cpp: unparsed arguments ${__cppopts_UNPARSED_ARGUMENTS}")
    endif()

    if(__cppopts_WITH_RUNTIME)
        set(CPP_USE_RT 1)
    endif()
    if(__cppopts_WITH_RTTI)
        if(MSVC)
            replace_compile_flags("/GR-" "/GR")
        else()
            replace_compile_flags_language("-fno-rtti" "-frtti" "CXX")
        endif()
    endif()
    if(__cppopts_WITH_EXCEPTIONS)
        if(MSVC)
            replace_compile_flags("/EHs-c-" "/EHsc")
        else()
            replace_compile_flags_language("-fno-exceptions" "-fexceptions" "CXX")
        endif()
    endif()
    if(__cppopts_WITH_STL)
        set(CPP_USE_STL 1)
        if(MSVC)
            add_definitions(-DNATIVE_CPP_INCLUDE=${REACTOS_SOURCE_DIR}/sdk/include/c++)
            include_directories(${REACTOS_SOURCE_DIR}/sdk/include/c++/stlport)
        else()
            replace_compile_flags("-nostdinc" " ")
        endif()
    endif()

    set(IS_CPP 1)
endmacro()

function(add_dependency_node _node)
    if(GENERATE_DEPENDENCY_GRAPH)
        get_target_property(_type ${_node} TYPE)
        if(_type MATCHES SHARED_LIBRARY OR ${_node} MATCHES ntoskrnl)
            file(APPEND ${REACTOS_BINARY_DIR}/dependencies.graphml "    <node id=\"${_node}\"/>\n")
        endif()
     endif()
endfunction()

function(add_dependency_edge _source _target)
    if(GENERATE_DEPENDENCY_GRAPH)
        get_target_property(_type ${_source} TYPE)
        if(_type MATCHES SHARED_LIBRARY)
            file(APPEND ${REACTOS_BINARY_DIR}/dependencies.graphml "    <edge source=\"${_source}\" target=\"${_target}\"/>\n")
        endif()
    endif()
endfunction()

function(add_dependency_header)
    file(APPEND ${REACTOS_BINARY_DIR}/dependencies.graphml "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<graphml>\n  <graph id=\"ReactOS dependencies\" edgedefault=\"directed\">\n")
endfunction()

function(add_dependency_footer)
    add_dependency_node(ntdll)
    file(APPEND ${REACTOS_BINARY_DIR}/dependencies.graphml "  </graph>\n</graphml>\n")
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
        get_target_property(_CD_FILE ${_CD_TARGET} LOCATION_${CMAKE_BUILD_TYPE})
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
            # add it in reactos.cab
            dir_to_num(${_CD_DESTINATION} _num)
            file(RELATIVE_PATH __relative_file ${REACTOS_SOURCE_DIR} ${_CD_FILE})
            file(APPEND ${REACTOS_BINARY_DIR}/boot/bootdata/packages/reactos.dff.dyn "\"${__relative_file}\" ${_num}\n")
            unset(__relative_file)
            # manage dependency - target level
            if(_CD_TARGET)
                add_dependencies(reactos_cab_inf ${_CD_TARGET})
            endif()
            # manage dependency - file level
            set_property(GLOBAL APPEND PROPERTY REACTOS_CAB_DEPENDS ${_CD_FILE})
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
    file(APPEND ${REACTOS_BINARY_DIR}/boot/bootcd.lst "${_filelist}")
    unset(_filelist)

    get_property(_filelist GLOBAL PROPERTY LIVECD_FILE_LIST)
    string(REPLACE ";" "\n" _filelist "${_filelist}")
    file(APPEND ${REACTOS_BINARY_DIR}/boot/livecd.lst "${_filelist}")
    unset(_filelist)

    get_property(_filelist GLOBAL PROPERTY HYBRIDCD_FILE_LIST)
    string(REPLACE ";" "\n" _filelist "${_filelist}")
    file(APPEND ${REACTOS_BINARY_DIR}/boot/hybridcd.lst "${_filelist}")
    unset(_filelist)

    get_property(_filelist GLOBAL PROPERTY BOOTCDREGTEST_FILE_LIST)
    string(REPLACE ";" "\n" _filelist "${_filelist}")
    file(APPEND ${REACTOS_BINARY_DIR}/boot/bootcdregtest.lst "${_filelist}")
    unset(_filelist)
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
        get_target_property(_target_excluded ${name} EXCLUDE_FROM_ALL)
        if(_target_excluded AND ${name} MATCHES "^lib.*")
            set_property(TARGET "${name}" PROPERTY FOLDER "Importlibs")
        else()
            string(SUBSTRING ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR_LENGTH} -1 CMAKE_CURRENT_SOURCE_DIR_RELATIVE)
            set_property(TARGET "${name}" PROPERTY FOLDER "${CMAKE_CURRENT_SOURCE_DIR_RELATIVE}")
        endif()
    endfunction()

    function(add_executable name)
        _add_executable(${name} ${ARGN})
        string(SUBSTRING ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR_LENGTH} -1 CMAKE_CURRENT_SOURCE_DIR_RELATIVE)
        set_property(TARGET "${name}" PROPERTY FOLDER "${CMAKE_CURRENT_SOURCE_DIR_RELATIVE}")
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
        if("${LIB}" MATCHES "msvcrt")
            add_target_compile_definitions(${_module} _DLL __USE_CRTIMP)
            target_link_libraries(${_module} msvcrtex)
        endif()
        target_link_libraries(${_module} lib${LIB})
        add_dependencies(${_module} lib${LIB})
        add_dependency_edge(${_module} ${LIB})
    endforeach()
endfunction()

function(set_module_type MODULE TYPE)
    cmake_parse_arguments(__module "UNICODE" "IMAGEBASE" "ENTRYPOINT" ${ARGN})

    if(__module_UNPARSED_ARGUMENTS)
        message(STATUS "set_module_type : unparsed arguments ${__module_UNPARSED_ARGUMENTS}, module : ${MODULE}")
    endif()

    # Add the module to the module group list, if it is defined
    if(DEFINED CURRENT_MODULE_GROUP)
        set_property(GLOBAL APPEND PROPERTY ${CURRENT_MODULE_GROUP}_MODULE_LIST "${MODULE}")
    endif()

    # Set subsystem. Also take this as an occasion
    # to error out if someone gave a non existing type
    if((${TYPE} STREQUAL nativecui) OR (${TYPE} STREQUAL nativedll)
            OR (${TYPE} STREQUAL kernelmodedriver) OR (${TYPE} STREQUAL wdmdriver) OR (${TYPE} STREQUAL kerneldll))
        set(__subsystem native)
    elseif(${TYPE} STREQUAL win32cui)
        set(__subsystem console)
    elseif(${TYPE} STREQUAL win32gui)
        set(__subsystem windows)
    elseif(NOT ((${TYPE} STREQUAL win32dll) OR (${TYPE} STREQUAL win32ocx)
            OR (${TYPE} STREQUAL cpl) OR (${TYPE} STREQUAL module)))
        message(FATAL_ERROR "Unknown type ${TYPE} for module ${MODULE}")
    endif()

    if(DEFINED __subsystem)
        set_subsystem(${MODULE} ${__subsystem})
    endif()

    #set unicode definitions
    if(__module_UNICODE)
        add_target_compile_definitions(${MODULE} UNICODE _UNICODE)
    endif()

    # set entry point
    if(__module_ENTRYPOINT OR (__module_ENTRYPOINT STREQUAL "0"))
        list(GET __module_ENTRYPOINT 0 __entrypoint)
        list(LENGTH __module_ENTRYPOINT __length)
        if(${__length} EQUAL 2)
            list(GET __module_ENTRYPOINT 1 __entrystack)
        elseif(NOT ${__length} EQUAL 1)
            message(FATAL_ERROR "Wrong arguments for ENTRYPOINT parameter of set_module_type : ${__module_ENTRYPOINT}")
        endif()
        unset(__length)
    elseif(${TYPE} STREQUAL nativecui)
        set(__entrypoint NtProcessStartup)
        set(__entrystack 4)
    elseif(${TYPE} STREQUAL win32cui)
        if(__module_UNICODE)
            set(__entrypoint wmainCRTStartup)
        else()
            set(__entrypoint mainCRTStartup)
        endif()
    elseif(${TYPE} STREQUAL win32gui)
        if(__module_UNICODE)
            set(__entrypoint wWinMainCRTStartup)
        else()
            set(__entrypoint WinMainCRTStartup)
        endif()
    elseif((${TYPE} STREQUAL win32dll) OR (${TYPE} STREQUAL win32ocx)
            OR (${TYPE} STREQUAL cpl))
        set(__entrypoint DllMainCRTStartup)
        set(__entrystack 12)
    elseif((${TYPE} STREQUAL kernelmodedriver) OR (${TYPE} STREQUAL wdmdriver))
        set(__entrypoint DriverEntry)
        set(__entrystack 8)
    elseif(${TYPE} STREQUAL nativedll)
        set(__entrypoint DllMain)
        set(__entrystack 12)
    elseif(${TYPE} STREQUAL module)
        set(__entrypoint 0)
    endif()

    if(DEFINED __entrypoint)
        if(DEFINED __entrystack)
            set_entrypoint(${MODULE} ${__entrypoint} ${__entrystack})
        else()
            set_entrypoint(${MODULE} ${__entrypoint})
        endif()
    endif()

    #set base address
    if(__module_IMAGEBASE)
        set_image_base(${MODULE} ${__module_IMAGEBASE})
    elseif(${TYPE} STREQUAL win32dll)
        if(DEFINED baseaddress_${MODULE})
            set_image_base(${MODULE} ${baseaddress_${MODULE}})
        else()
            message(STATUS "${MODULE} has no base address")
        endif()
    elseif((${TYPE} STREQUAL kernelmodedriver) OR (${TYPE} STREQUAL wdmdriver) OR (${TYPE} STREQUAL kerneldll))
        set_image_base(${MODULE} 0x00010000)
    endif()

    # Now do some stuff which is specific to each type
    if((${TYPE} STREQUAL kernelmodedriver) OR (${TYPE} STREQUAL wdmdriver) OR (${TYPE} STREQUAL kerneldll))
        add_dependencies(${MODULE} bugcodes xdk)
        if((${TYPE} STREQUAL kernelmodedriver) OR (${TYPE} STREQUAL wdmdriver))
            set_target_properties(${MODULE} PROPERTIES SUFFIX ".sys")
        endif()
    endif()

    if(${TYPE} STREQUAL win32ocx)
        set_target_properties(${MODULE} PROPERTIES SUFFIX ".ocx")
    endif()

    if(${TYPE} STREQUAL cpl)
        set_target_properties(${MODULE} PROPERTIES SUFFIX ".cpl")
    endif()

    # do compiler specific stuff
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
        list(APPEND __tmp_var -D${arg})
    endforeach()
    set(${OUTPUT_VAR} ${__tmp_var} PARENT_SCOPE)
endfunction()

if(NOT MSVC)
    function(add_object_library _target)
        add_library(${_target} OBJECT ${ARGN})
    endfunction()
else()
    function(add_object_library _target)
        add_library(${_target} ${ARGN})
    endfunction()
endif()

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
        string(REPLACE ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} _converted_file "${_file}")
        string(REPLACE ${_file_name} "${_file_name}_utf16" _converted_file ${_converted_file})
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

    # bootcd setup system hive
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/boot/bootdata/SETUPREG.HIV
        COMMAND native-mkhive -h:SETUPREG -d:${CMAKE_BINARY_DIR}/boot/bootdata ${CMAKE_BINARY_DIR}/boot/bootdata/hivesys_utf16.inf
        DEPENDS native-mkhive ${CMAKE_SOURCE_DIR}/boot/bootdata/hivesys.inf)

    add_custom_target(bootcd_hives
        DEPENDS ${CMAKE_BINARY_DIR}/boot/bootdata/SETUPREG.HIV)

    add_cd_file(
        FILE ${CMAKE_BINARY_DIR}/boot/bootdata/SETUPREG.HIV
        TARGET bootcd_hives
        DESTINATION reactos
        NO_CAB
        FOR bootcd regtest)

    # livecd hives
    list(APPEND _livecd_inf_files
        ${_registry_inf}
        ${CMAKE_SOURCE_DIR}/boot/bootdata/livecd.inf
        ${CMAKE_SOURCE_DIR}/boot/bootdata/hiveinst.inf)

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
        COMMAND native-mkhive -h:BCD -d:${CMAKE_BINARY_DIR}/boot/bootdata ${CMAKE_BINARY_DIR}/boot/bootdata/hivebcd_utf16.inf
        DEPENDS native-mkhive ${CMAKE_SOURCE_DIR}/boot/bootdata/hivebcd.inf)

    add_custom_target(bcd_hive
        DEPENDS ${CMAKE_BINARY_DIR}/boot/bootdata/BCD)

    add_cd_file(
        FILE ${CMAKE_BINARY_DIR}/boot/bootdata/BCD
        TARGET bcd_hive
        DESTINATION efi/boot
        NO_CAB
        FOR bootcd regtest livecd)

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

    if(NOT _ROSTESTS_FILE)
        get_target_property(_ROSTESTS_FILE ${_ROSTESTS_TARGET} LOCATION_${CMAKE_BUILD_TYPE})
    endif()

    if(NOT _ROSTESTS_RENAME)
        get_filename_component(_ROSTESTS_RENAME ${_ROSTESTS_FILE} NAME)
    endif()

    if(_ROSTESTS_SUBDIR)
        set(_ROSTESTS_SUBDIR "/${_ROSTESTS_SUBDIR}")
    endif()

    if(_ROSTESTS_TARGET)
        add_cd_file(TARGET ${_ROSTESTS_TARGET} FILE ${_ROSTESTS_FILE} DESTINATION "reactos/bin${_ROSTESTS_SUBDIR}" NAME_ON_CD ${_ROSTESTS_RENAME} FOR all)
    else()
        add_cd_file(FILE ${_ROSTESTS_FILE} DESTINATION "reactos/bin${_ROSTESTS_SUBDIR}" NAME_ON_CD ${_ROSTESTS_RENAME} FOR all)
    endif()

    if(DEFINED ENV{ROSTESTS_INSTALL})
        install(FILES ${_ROSTESTS_FILE} DESTINATION "$ENV{ROSTESTS_INSTALL}${_ROSTESTS_SUBDIR}" COMPONENT rostests RENAME ${_ROSTESTS_RENAME})
    endif()
endfunction()
