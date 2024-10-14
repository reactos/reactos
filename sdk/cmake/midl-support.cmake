
#idl files support
if(ARCH STREQUAL "i386")
    set(IDL_FLAGS /nologo /win32 /no_def_idir)
elseif(ARCH STREQUAL "amd64")
    set(IDL_FLAGS /nologo /amd64 /no_def_idir)
else()
    set(IDL_FLAGS /nologo /no_def_idir)
endif()

function(add_typelib)
    get_includes(_includes)
    get_defines(_defines)
    foreach(_idl_file ${ARGN})
        get_filename_component(_name_we ${_idl_file} NAME_WE)
        add_custom_command(
            OUTPUT ${_name_we}.tlb
            COMMAND midl ${_includes} ${_defines} ${IDL_FLAGS} /tlb ${_name_we}.tlb ${CMAKE_CURRENT_SOURCE_DIR}/${_idl_file}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_idl_file})
    endforeach()
endfunction()

function(add_idl_headers TARGET)
    get_includes(_includes)
    get_defines(_defines)
    foreach(_idl_file ${ARGN})
        get_filename_component(_name_we ${_idl_file} NAME_WE)
        add_custom_command(
            OUTPUT ${_name_we}.h
            COMMAND midl ${_includes} ${_defines} ${IDL_FLAGS} /h ${_name_we}.h /client none /server none /iid ${_name_we}_dummy_i.c ${CMAKE_CURRENT_SOURCE_DIR}/${_idl_file}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_idl_file})
        list(APPEND _target_dependencies ${_name_we}.h)
    endforeach()
    add_custom_target(${TARGET} DEPENDS ${_target_dependencies})
endfunction()

function(add_rpcproxy_files)
    get_includes(_includes)
    get_defines(_defines)
    set(_chain_dependency "")
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c PROPERTIES GENERATED TRUE)
    foreach(_idl_file ${ARGN})
        get_filename_component(_name_we ${_idl_file} NAME_WE)
        add_custom_command(
            OUTPUT ${_name_we}_p.c ${_name_we}_p.h
            COMMAND midl ${_includes} ${_defines} ${IDL_FLAGS} /client none /server none /proxy ${_name_we}_p.c /h ${_name_we}_p.h /dlldata proxy.dlldata.c ${CMAKE_CURRENT_SOURCE_DIR}/${_idl_file}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_idl_file} ${_chain_dependency})
        list(APPEND _chain_dependency ${CMAKE_CURRENT_BINARY_DIR}/${_name_we}_p.c)
        list(APPEND _chain_dependency ${CMAKE_CURRENT_BINARY_DIR}/${_name_we}_p.h)
        set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_idl_file})
    endforeach()
endfunction()

function(add_rpc_files _type)
    get_includes(_includes)
    get_defines(_defines)
    # Is it a client or server module?
    if(_type STREQUAL "server")
        set(_server_client /sstub)
        set(_suffix _s)
        set(_prevent_second_type /client none)
    elseif(_type STREQUAL "client")
        set(_server_client /cstub)
        set(_suffix _c)
        set(_prevent_second_type /server none)
    else()
        message(FATAL_ERROR "Please pass either server or client as argument to add_rpc_files")
    endif()
    foreach(_idl_file ${ARGN})
        if(NOT IS_ABSOLUTE ${_idl_file})
            set(_idl_file ${CMAKE_CURRENT_SOURCE_DIR}/${_idl_file})
        endif()
        get_filename_component(_name_we ${_idl_file} NAME_WE)
        set(_name_we ${_name_we}${_suffix})
        add_custom_command(
            OUTPUT ${_name_we}.c ${_name_we}.h
            COMMAND midl ${_includes} ${_defines} ${IDL_FLAGS} /h ${_name_we}.h ${_server_client} ${_name_we}.c ${_prevent_second_type} ${_idl_file}
            DEPENDS ${_idl_file})
    endforeach()
endfunction()

function(generate_idl_iids)
    foreach(_idl_file ${ARGN})
        get_includes(_includes)
        get_defines(_defines)

        if(NOT IS_ABSOLUTE ${_idl_file})
            set(_idl_file "${CMAKE_CURRENT_SOURCE_DIR}/${_idl_file}")
        endif()

        get_filename_component(_name_we ${_idl_file} NAME_WE)
        add_custom_command(
            OUTPUT ${_name_we}_i.c ${_name_we}_i.h
            COMMAND midl ${_includes} ${_defines} ${IDL_FLAGS} /h ${_name_we}_i.h /client none /server none /iid ${_name_we}_i.c /proxy ${_name_we}_dummy_p.c ${_idl_file}
            DEPENDS ${_idl_file})
        set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${_name_we}_i.c PROPERTIES GENERATED TRUE)
    endforeach()
endfunction()

function(add_iid_library _target)

    foreach(_idl_file ${ARGN})
        generate_idl_iids(${_idl_file})
        get_filename_component(_name_we ${_idl_file} NAME_WE)
        list(APPEND _iid_sources ${CMAKE_CURRENT_BINARY_DIR}/${_name_we}_i.c)
    endforeach()
    add_library(${_target} ${_iid_sources})

    # for wtypes.h
    add_dependencies(${_target} psdk)

    set_target_properties(${_target} PROPERTIES EXCLUDE_FROM_ALL TRUE)
endfunction()
