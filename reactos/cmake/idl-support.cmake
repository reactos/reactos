
#idl files support
if(MSVC)
    set(IDL_COMPILER midl)
    set(IDL_HEADER_ARG /h) #.h
    set(IDL_HEADER_ARG2 /h) #.h
    set(IDL_TYPELIB_ARG /tlb) #.tlb
    set(IDL_SERVER_ARG /sstub) #.c for stub server library
    set(IDL_CLIENT_ARG /cstub) #.c for stub client library
    set(IDL_PROXY_ARG /proxy)
    set(IDL_INTERFACE_ARG /iid)
    if(ARCH MATCHES i386)
        set(IDL_FLAGS /nologo /win32)
    elseif(ARCH MATCHES amd64)
        set(IDL_FLAGS /nologo /amd64)
    else()
        set(IDL_FLAGS /nologo)
    endif()
    set(IDL_DEPENDS "")
else()
    set(IDL_COMPILER native-widl)
    set(IDL_HEADER_ARG -h -o) #.h
    set(IDL_HEADER_ARG2 -h -H) #.h
    set(IDL_TYPELIB_ARG -t -o) #.tlb
    set(IDL_SERVER_ARG -Oif -s -o) #.c for server library
    set(IDL_CLIENT_ARG -Oif -c -o) #.c for stub client library
    set(IDL_PROXY_ARG -p -o)
    set(IDL_INTERFACE_ARG -u -o)
    if(ARCH MATCHES i386)
        set(IDL_FLAGS -m32 --win32)
    elseif(ARCH MATCHES amd64)
        set(IDL_FLAGS -m64 --win64)
    else()
        set(IDL_FLAGS "")
    endif()
    set(IDL_DEPENDS native-widl)
endif()


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

function(add_typelib TARGET)
    get_includes(INCLUDES)
    get_defines(DEFINES)
    foreach(FILE ${ARGN})
        get_filename_component(NAME ${FILE} NAME_WE)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.tlb
            COMMAND ${IDL_COMPILER} ${INCLUDES} ${DEFINES} ${IDL_FLAGS} ${IDL_TYPELIB_ARG} ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.tlb ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FILE} ${IDL_DEPENDS})
        list(APPEND OBJECTS ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.tlb)
    endforeach()
    add_custom_target(${TARGET} ALL DEPENDS ${OBJECTS})
endfunction()

function(add_idl_headers TARGET)
    get_includes(INCLUDES)
    get_defines(DEFINES)
    foreach(FILE ${ARGN})
        get_filename_component(NAME ${FILE} NAME_WE)
        set(HEADER ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.h)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.h
            COMMAND ${IDL_COMPILER} ${INCLUDES} ${DEFINES} ${IDL_FLAGS} ${IDL_HEADER_ARG} ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.h ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FILE} ${IDL_DEPENDS})
        list(APPEND HEADERS ${HEADER})
    endforeach()
    add_custom_target(${TARGET} DEPENDS ${HEADERS})
endfunction()

function(add_rpcproxy_files)
    get_includes(INCLUDES)
    get_defines(DEFINES)

    if(MSVC)
        set(DLLDATA_ARG /dlldata ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c)
        set(DLLDATA_DEPENDENCIES "")
    endif()
    foreach(FILE ${ARGN})
        get_filename_component(NAME ${FILE} NAME_WE)
        if(MSVC)
            set(DLLDATA_DEPENDENCIES ${DLLDATA_DEPENDENCIES} ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_p.c)
        else()
            list(APPEND IDLS ${CMAKE_CURRENT_SOURCE_DIR}/${FILE})
        endif()
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_p.c ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_p.h
            COMMAND ${IDL_COMPILER} ${INCLUDES} ${DEFINES} ${IDL_FLAGS} ${IDL_PROXY_ARG} ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_p.c ${IDL_HEADER_ARG2} ${NAME}_p.h ${CMAKE_CURRENT_SOURCE_DIR}/${FILE} ${DLLDATA_ARG}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FILE} ${IDL_DEPENDS})
    endforeach()

    # Extra pass to generate dlldata
    if(MSVC)
        #touch it, so we're sure it's older than its dependencies
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
            COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
            DEPENDS ${DLLDATA_DEPENDENCIES})
        set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c PROPERTIES GENERATED TRUE)
    else()
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
            COMMAND ${IDL_COMPILER} ${INCLUDES} ${DEFINES} ${IDL_FLAGS} --dlldata-only -o ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c ${IDLS}
            DEPENDS ${IDLS} ${IDL_DEPENDS})
    endif()
endfunction()

function(add_rpc_files __type)
    get_includes(INCLUDES)
    get_defines(DEFINES)
    # Is it a client or server module?
    if(__type STREQUAL server)
        set(__server_client ${IDL_SERVER_ARG})
        set(__suffix _s)
    elseif(__type STREQUAL client)
        set(__server_client ${IDL_CLIENT_ARG})
        set(__suffix _c)
    else()
        message(FATAL_ERROR "Please pass either server or client as argument to add_rpc_files")
    endif()
    foreach(FILE ${ARGN})
        get_filename_component(__name ${FILE} NAME_WE)
        set(__name ${__name}${__suffix})
        if(NOT IS_ABSOLUTE ${FILE})
            set(FILE ${CMAKE_CURRENT_SOURCE_DIR}/${FILE})
        endif()
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${__name}.c ${CMAKE_CURRENT_BINARY_DIR}/${__name}.h
            COMMAND ${IDL_COMPILER} ${INCLUDES} ${DEFINES} ${IDL_FLAGS} ${IDL_HEADER_ARG2} ${CMAKE_CURRENT_BINARY_DIR}/${__name}.h ${__server_client} ${CMAKE_CURRENT_BINARY_DIR}/${__name}.c ${FILE}
            DEPENDS ${FILE} ${IDL_DEPENDS})
    endforeach()
endfunction()

function(generate_idl_iids IDL_FILE)
    get_filename_component(FILE ${IDL_FILE} NAME)
    if(FILE STREQUAL "${IDL_FILE}")
        set(IDL_FILE_FULL "${CMAKE_CURRENT_SOURCE_DIR}/${IDL_FILE}")
    else()
        set(IDL_FILE_FULL ${IDL_FILE})
    endif()
    get_includes(INCLUDES)
    get_defines(DEFINES)
    get_filename_component(NAME ${IDL_FILE} NAME_WE)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_i.c
        COMMAND ${IDL_COMPILER} ${INCLUDES} ${DEFINES} ${IDL_FLAGS} ${IDL_INTERFACE_ARG} ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_i.c ${IDL_FILE_FULL}
        DEPENDS ${IDL_FILE_FULL} ${IDL_DEPENDS})
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${NAME}_i.c PROPERTIES GENERATED TRUE)
endfunction()

function(add_iid_library TARGET)
    foreach(IDL_FILE ${ARGN})
        get_filename_component(NAME ${IDL_FILE} NAME_WE)
        generate_idl_iids(${IDL_FILE})
        list(APPEND IID_SOURCES ${NAME}_i.c)
    endforeach()
    add_library(${TARGET} ${IID_SOURCES})
	add_dependencies(${TARGET} psdk)
    set_target_properties(${TARGET} PROPERTIES EXCLUDE_FROM_ALL TRUE)
endfunction()
