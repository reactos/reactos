
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86")
    add_definitions(-D__i386__)
endif()

add_definitions(-Dinline=__inline)

if(NOT CMAKE_CROSSCOMPILING)


else()

add_definitions(/GS- /Zl /Zi)
add_definitions(-Dinline=__inline -D__STDC__=1)

IF(${_MACHINE_ARCH_FLAG} MATCHES X86)
  SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
  SET (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
  SET (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO")
ENDIF()

link_directories("${REACTOS_BINARY_DIR}/importlibs" ${REACTOS_BINARY_DIR}/lib/3rdparty/mingw)

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
        set_entrypoint(${MODULE} mainCRTStartup)
    endif ()
    if(${TYPE} MATCHES win32dll)
        # Need this only because mingw library is broken
        set_entrypoint(${MODULE} DllMainCRTStartup@12)
		if(DEFINED baseaddress_${MODULE})
			set_image_base(${MODULE} ${baseaddress_${MODULE}})
		else()
			message(STATUS "${MODULE} has no base address")
		endif()
		target_link_libraries(${MODULE} mingw_common mingw_dllmain)
    endif()

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


macro(add_importlib_target _def_file)
    get_filename_component(_name ${_def_file} NAME_WE)
    add_custom_command(
        OUTPUT {CMAKE_BINARY_DIR}/importlibs/lib${_name}.lib
        COMMAND LINK /LIB /MACHINE:X86 /DEF:${_def_file} /OUT:${CMAKE_BINARY_DIR}/importlibs/lib${_name}.lib
        DEPENDS ${_def_file}
    )
    add_custom_target(
        lib${_name}
        DEPENDS {CMAKE_BINARY_DIR}/importlibs/lib${_name}.lib
    )
endmacro()

macro(add_importlibs MODULE)
    foreach(LIB ${ARGN})
        target_link_libraries(${MODULE} ${CMAKE_BINARY_DIR}/importlibs/lib${LIB}.lib)
        add_dependencies(${MODULE} lib${LIB})
    endforeach()
endmacro()

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/importlibs)

#pseh workaround
set(PSEH_LIB "")

endif()
