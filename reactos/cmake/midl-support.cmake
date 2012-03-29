
#idl files support
if(ARCH MATCHES i386)
    set(IDL_FLAGS /nologo /win32 /no_def_idir)
elseif(ARCH MATCHES amd64)
    set(IDL_FLAGS /nologo /amd64 /no_def_idir)
else()
    set(IDL_FLAGS /nologo /no_def_idir)
endif()

function(add_typelib)
    get_includes(INCLUDES)
    get_defines(DEFINES)
    foreach(FILE ${ARGN})
        get_filename_component(NAME ${FILE} NAME_WE)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.tlb
            COMMAND midl ${INCLUDES} ${DEFINES} ${IDL_FLAGS} /tlb ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.tlb ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FILE})
    endforeach()
endfunction()

function(add_idl_headers TARGET)
    get_includes(INCLUDES)
    get_defines(DEFINES)
    foreach(FILE ${ARGN})
        get_filename_component(NAME ${FILE} NAME_WE)
        set(HEADER ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.h)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.h
            COMMAND midl ${INCLUDES} ${DEFINES} ${IDL_FLAGS} /h ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.h ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FILE})
        list(APPEND HEADERS ${HEADER})
    endforeach()
    add_custom_target(${TARGET} DEPENDS ${HEADERS})
endfunction()

function(add_rpcproxy_files)
    get_includes(INCLUDES)
    get_defines(DEFINES)
    set(OUTPUT_FILES "")
    foreach(FILE ${ARGN})
        get_filename_component(NAME ${FILE} NAME_WE)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_p.c ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_p.h ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
            COMMAND midl ${INCLUDES} ${DEFINES} ${IDL_FLAGS} /proxy ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_p.c /h ${NAME}_p.h ${CMAKE_CURRENT_SOURCE_DIR}/${FILE} /dlldata ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FILE} ${OUTPUT_FILES})
        list(APPEND OUTPUT_FILES ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_p.c)
        list(APPEND OUTPUT_FILES ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_p.h)
    endforeach()
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c PROPERTIES GENERATED TRUE)
endfunction()

function(add_rpc_files __type)
    get_includes(INCLUDES)
    get_defines(DEFINES)
    # Is it a client or server module?
    if(__type STREQUAL server)
        set(__server_client /sstub)
        set(__suffix _s)
    elseif(__type STREQUAL client)
        set(__server_client /cstub)
        set(__suffix _c)
    else()
        message(FATAL_ERROR "Please pass either server or client as argument to add_rpc_files")
    endif()
    foreach(FILE ${ARGN})
        get_filename_component(__name ${FILE} NAME_WE)
        set(__name ${CMAKE_CURRENT_BINARY_DIR}/${__name}${__suffix})
        if(NOT IS_ABSOLUTE ${FILE})
            set(FILE ${CMAKE_CURRENT_SOURCE_DIR}/${FILE})
        endif()
        add_custom_command(
            OUTPUT ${__name}.c ${__name}.h
            COMMAND midl ${INCLUDES} ${DEFINES} ${IDL_FLAGS} /h ${__name}.h ${__server_client} ${__name}.c ${FILE}
            DEPENDS ${FILE})
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
        COMMAND midl ${INCLUDES} ${DEFINES} ${IDL_FLAGS} /iid ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_i.c ${IDL_FILE_FULL}
        DEPENDS ${IDL_FILE_FULL})
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
