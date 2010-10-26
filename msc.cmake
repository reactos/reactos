
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86")
    add_definitions(-D__i386__)
endif()

add_definitions(-Dinline=__inline)

if(NOT CMAKE_CROSSCOMPILING)



else()

add_definitions(/GS- /Zl /Zi)
add_definitions(-Dinline=__inline -D__STDC__=1)

set(CMAKE_RC_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")


macro(set_entrypoint MODULE ENTRYPOINT)
    if(${ENTRYPOINT} STREQUAL "0")
        set(NEW_LINKER_FLAGS "/ENTRY:0")
    else()
        set(NEW_LINKER_FLAGS "/ENTRY:${ENTRYPOINT}")
    endif()
    get_target_property(LINKER_FLAGS ${MODULE} LINK_FLAGS)
    if(LINKER_FLAGS)
        set(NEW_LINKER_FLAGS "${LINKER_FLAGS} ${NEW_LINKER_FLAGS}")
    endif()
    set_target_properties(${MODULE} PROPERTIES LINK_FLAGS ${NEW_LINKER_FLAGS})
endmacro()

macro(set_subsystem MODULE SUBSYSTEM)
    set(NEW_LINKER_FLAGS "/subsystem:${SUBSYSTEM}")
    get_target_property(LINKER_FLAGS ${MODULE} LINK_FLAGS)
    if(LINKER_FLAGS)
        set(NEW_LINKER_FLAGS "${LINKER_FLAGS} ${NEW_LINKER_FLAGS}")
    endif()
    set_target_properties(${MODULE} PROPERTIES LINK_FLAGS ${NEW_LINKER_FLAGS})
endmacro()

macro(set_image_base MODULE IMAGE_BASE)
    set(NEW_LINKER_FLAGS "/BASE:${IMAGE_BASE}")
    get_target_property(LINKER_FLAGS ${MODULE} LINK_FLAGS)
    if(LINKER_FLAGS)
        set(NEW_LINKER_FLAGS "${LINKER_FLAGS} ${NEW_LINKER_FLAGS}")
    endif()
    set_target_properties(${MODULE} PROPERTIES LINK_FLAGS ${NEW_LINKER_FLAGS})
endmacro()

macro(add_importlibs MODULE)
    foreach(LIB ${ARGN})
        target_link_libraries(${MODULE} ${LIB}.LIB)
    endforeach()
endmacro()

macro(set_module_type MODULE TYPE)
    add_dependencies(${MODULE} psdk buildno_header)
    if(${TYPE} MATCHES nativecui)
        set_subsystem(${MODULE} native)
        add_importlibs(${MODULE} ntdll)
    endif()
    if (${TYPE} MATCHES win32gui)
        set_subsystem(${MODULE} windows)
    endif ()
    if (${TYPE} MATCHES win32cui)
        set_subsystem(${MODULE} console)
    endif ()
endmacro()

macro(set_unicode)
    add_definitions(-DUNICODE -D_UNICODE)
endmacro()

set(CMAKE_C_FLAGS_DEBUG_INIT "/D_DEBUG /MDd /Zi  /Ob0 /Od")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "/D_DEBUG /MDd /Zi /Ob0 /Od")

macro(set_rc_compiler)
# dummy, this workaround is only needed in mingw due to lack of RC support in cmake
endmacro()

#idl files support
set(IDL_COMPILER midl)
set(IDL_FLAGS /win32)
set(IDL_HEADER_ARG /h) #.h
set(IDL_TYPELIB_ARG /tlb) #.tlb
set(IDL_SERVER_ARG /sstub) #.c for stub server library
set(IDL_CLIENT_ARG /cstub) #.c for stub client library

endif()