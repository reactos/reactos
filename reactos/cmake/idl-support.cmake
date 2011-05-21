
#idl files support
if(MSVC)
    set(IDL_COMPILER midl)
    set(IDL_HEADER_ARG /h) #.h
    set(IDL_HEADER_ARG2 /header) #.h
    set(IDL_TYPELIB_ARG /tlb) #.tlb
    set(IDL_SERVER_ARG /sstub) #.c for stub server library
    set(IDL_CLIENT_ARG /cstub) #.c for stub client library
    set(IDL_PROXY_ARG /proxy)
    set(IDL_INTERFACE_ARG /iid)
    if(ARCH MATCHES i386)
        set(IDL_FLAGS /win32)
    elseif(ARCH MATCHES amd64)
        set(IDL_FLAGS /amd64)
    else()
        set(IDL_FLAGS "")
    endif()
else()
    set(IDL_COMPILER native-widl)
    set(IDL_HEADER_ARG -h -o) #.h
    set(IDL_HEADER_ARG2 -h -H) #.h
    set(IDL_TYPELIB_ARG -t -o) #.tlb
    set(IDL_SERVER_ARG -s -o) #.c for server library
    set(IDL_CLIENT_ARG -c -o) #.c for stub client library
    set(IDL_PROXY_ARG -p -o)
    set(IDL_INTERFACE_ARG -u -o)
    if(ARCH MATCHES i386)
        set(IDL_FLAGS -m32 --win32)
    elseif(ARCH MATCHES amd64)
        set(IDL_FLAGS -m64 --win64)
    else()
        set(IDL_FLAGS "")
    endif()
endif()


macro(get_includes OUTPUT_VAR)
    set(${OUTPUT_VAR} "")
    get_directory_property(_includes INCLUDE_DIRECTORIES)
    foreach(arg ${_includes})
        set(${OUTPUT_VAR} -I${arg} ${${OUTPUT_VAR}})
    endforeach()
endmacro()

macro(get_defines OUTPUT_VAR)
    set(${OUTPUT_VAR} "")
    get_directory_property(_defines COMPILE_DEFINITIONS)
    foreach(arg ${_defines})
        set(${OUTPUT_VAR} ${${OUTPUT_VAR}} -D${arg})
    endforeach()
endmacro()

macro(add_typelib TARGET)
    get_includes(INCLUDES)
    get_defines(DEFINES)
    foreach(FILE ${ARGN})
        get_filename_component(NAME ${FILE} NAME_WE)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.tlb
            COMMAND ${IDL_COMPILER} ${INCLUDES} ${DEFINES} ${IDL_FLAGS} ${IDL_TYPELIB_ARG} ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.tlb ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FILE})
        list(APPEND OBJECTS ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.tlb)
    endforeach()
    add_custom_target(${TARGET} ALL DEPENDS ${OBJECTS})
endmacro()

macro(add_idl_headers TARGET)
    get_includes(INCLUDES)
    get_defines(DEFINES)
    foreach(FILE ${ARGN})
        get_filename_component(NAME ${FILE} NAME_WE)
        set(HEADER ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.h)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.h
            COMMAND ${IDL_COMPILER} ${INCLUDES} ${DEFINES} ${IDL_FLAGS} ${IDL_HEADER_ARG} ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.h ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FILE})
        list(APPEND HEADERS ${HEADER})
    endforeach()
    add_custom_target(${TARGET} ALL DEPENDS ${HEADERS})
endmacro()

macro(add_rpcproxy_files)
    get_includes(INCLUDES)
    get_defines(DEFINES)

    if(MSVC)
        set(DLLDATA_ARG /dlldata ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c)
    endif()
    foreach(FILE ${ARGN})
        get_filename_component(NAME ${FILE} NAME_WE)
        if(NOT MSVC)
            list(APPEND IDLS ${CMAKE_CURRENT_SOURCE_DIR}/${FILE})
        endif()
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_p.c ${NAME}_p.h
            COMMAND ${IDL_COMPILER} ${INCLUDES} ${DEFINES} ${IDL_FLAGS} ${IDL_PROXY_ARG} ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_p.c ${IDL_HEADER_ARG2} ${NAME}_p.h ${CMAKE_CURRENT_SOURCE_DIR}/${FILE} ${DLLDATA_ARG}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FILE})
    endforeach()

    # Extra pass to generate dlldata
    if(MSVC)
        #nobody told how to generate it, so mark it as generated
        set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c PROPERTIES GENERATED TRUE)
    else()
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
            COMMAND ${IDL_COMPILER} ${INCLUDES} ${DEFINES} ${IDL_FLAGS} --dlldata-only -o ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c ${IDLS}
            DEPENDS ${IDLS})
    endif()
endmacro()

macro(add_rpc_library TARGET)
    get_includes(INCLUDES)
    get_defines(DEFINES)
    foreach(FILE ${ARGN})
        get_filename_component(NAME ${FILE} NAME_WE)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_s.c ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_s.h
            COMMAND ${IDL_COMPILER} ${INCLUDES} ${DEFINES} ${IDL_FLAGS} ${IDL_HEADER_ARG2} ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_s.h ${IDL_SERVER_ARG} ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_s.c ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FILE})
        list(APPEND server_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_s.c)

        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_c.c ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_c.h
            COMMAND ${IDL_COMPILER} ${INCLUDES} ${DEFINES} ${IDL_FLAGS} ${IDL_HEADER_ARG2} ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_c.h ${IDL_CLIENT_ARG} ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_c.c ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FILE})
        list(APPEND client_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_c.c)
    endforeach()
    add_library(${TARGET} ${client_SOURCES} ${server_SOURCES})
    add_dependencies(${TARGET} psdk)
endmacro()

macro(generate_idl_iids IDL_FILE)
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
        DEPENDS ${IDL_FILE_FULL})
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${NAME}_i.c PROPERTIES GENERATED TRUE)
endmacro()

macro(add_iid_library TARGET)
    foreach(IDL_FILE ${ARGN})
        get_filename_component(NAME ${IDL_FILE} NAME_WE)
        generate_idl_iids(${IDL_FILE})
        list(APPEND IID_SOURCES ${NAME}_i.c)
    endforeach()
    add_library(${TARGET} ${IID_SOURCES})
	add_dependencies(${TARGET} psdk)
endmacro()
