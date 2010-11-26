
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

MACRO(add_pch _target_name _header_filename _src_list)
    get_filename_component(FILE ${_header_filename} NAME)
    set(_gch_filename "${_target_name}_${FILE}.gch")
    add_custom_command(
        OUTPUT ${_gch_filename}
        COMMAND echo Ignoring precompiled header
        DEPENDS ${_header_filename})
ENDMACRO(add_pch _target_name _header_filename _src_list)

macro(add_linkerflag MODULE _flag)
    set(NEW_LINKER_FLAGS ${_flag})
    get_target_property(LINKER_FLAGS ${MODULE} LINK_FLAGS)
    if(LINKER_FLAGS)
        set(NEW_LINKER_FLAGS "${LINKER_FLAGS} ${NEW_LINKER_FLAGS}")
    endif()
    set_target_properties(${MODULE} PROPERTIES LINK_FLAGS ${NEW_LINKER_FLAGS})
endmacro()

macro(set_entrypoint MODULE ENTRYPOINT)
    if(${ENTRYPOINT} STREQUAL "0")
        add_linkerflag(${MODULE} "/NOENTRY")
    else()
        add_linkerflag(${MODULE} "/ENTRY:${ENTRYPOINT}")
    endif()
endmacro()

macro(set_subsystem MODULE SUBSYSTEM)
    add_linkerflag(${MODULE} "/subsystem:${SUBSYSTEM}")
endmacro()

macro(set_image_base MODULE IMAGE_BASE)
    add_linkerflag(${MODULE} "/BASE:${IMAGE_BASE}")
endmacro()

macro(set_module_type MODULE TYPE)
    add_dependencies(${MODULE} psdk buildno_header)
    if(${TYPE} MATCHES nativecui)
        set_subsystem(${MODULE} native)
        set_entrypoint(${MODULE} NtProcessStartup@4)
    endif()
    if (${TYPE} MATCHES win32gui)
        set_subsystem(${MODULE} windows)
        set_entrypoint(${MODULE} WinMainCRTStartup)
        if(IS_UNICODE)
            target_link_libraries(${MODULE} mingw_wmain)
        else()
            target_link_libraries(${MODULE} mingw_main)
        endif()
		target_link_libraries(${MODULE} mingw_common msvcsup)
    endif ()
    if (${TYPE} MATCHES win32cui)
        set_subsystem(${MODULE} console)
        set_entrypoint(${MODULE} mainCRTStartup)
        if(IS_UNICODE)
            target_link_libraries(${MODULE} mingw_wmain)
        else()
            target_link_libraries(${MODULE} mingw_main)
        endif()
		target_link_libraries(${MODULE} mingw_common msvcsup)
    endif ()
    if(${TYPE} MATCHES win32dll)
        # Need this only because mingw library is broken
        set_entrypoint(${MODULE} DllMainCRTStartup@12)
		if(DEFINED baseaddress_${MODULE})
			set_image_base(${MODULE} ${baseaddress_${MODULE}})
		else()
			message(STATUS "${MODULE} has no base address")
		endif()
		target_link_libraries(${MODULE} mingw_common mingw_dllmain msvcsup)
        add_linkerflag(${MODULE} "/DLL")
    endif()
    if(${TYPE} MATCHES win32ocx)
        set_entrypoint(${MODULE} DllMainCRTStartup@12)
        set_target_properties(${MODULE} PROPERTIES SUFFIX ".ocx")
        target_link_libraries(${MODULE} mingw_common mingw_dllmain msvcsup)
        add_linkerflag(${MODULE} "/DLL")
    endif()
    if(${TYPE} MATCHES cpl)
        set_entrypoint(${MODULE} DllMainCRTStartup@12)
        set_target_properties(${MODULE} PROPERTIES SUFFIX ".cpl")
        target_link_libraries(${MODULE} mingw_common mingw_dllmain msvcsup)
        add_linkerflag(${MODULE} "/DLL")
    endif()
	if(${TYPE} MATCHES kernelmodedriver)
	    set_target_properties(${MODULE} PROPERTIES SUFFIX ".sys")
	    set_entrypoint(${MODULE} DriverEntry@8)
		set_subsystem(${MODULE} native)
        set_image_base(${MODULE} 0x00010000)
        add_linkerflag(${MODULE} "/DRIVER")
		add_dependencies(${MODULE} bugcodes)
	endif()

endmacro()

macro(set_unicode)
   add_definitions(-DUNICODE -D_UNICODE)
   set(IS_UNICODE 1)
endmacro()

set(CMAKE_C_FLAGS_DEBUG_INIT "/D_DEBUG /MDd /Zi  /Ob0 /Od")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "/D_DEBUG /MDd /Zi /Ob0 /Od")

macro(set_rc_compiler)
# dummy, this workaround is only needed in mingw due to lack of RC support in cmake
endmacro()

#idl files support
set(IDL_COMPILER midl)
set(IDL_FLAGS /win32 /Dstrict_context_handle=)
set(IDL_HEADER_ARG /h) #.h
set(IDL_TYPELIB_ARG /tlb) #.tlb
set(IDL_SERVER_ARG /sstub) #.c for stub server library
set(IDL_CLIENT_ARG /cstub) #.c for stub client library

# Thanks MS for creating a stupid linker
macro(add_importlib_target _spec_file)
    get_filename_component(_name ${_spec_file} NAME_WE)

    # Generate the asm stub file
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.asm
        COMMAND native-spec2def -s ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file} ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.asm
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file})

    # Generate a the export def file
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_exp.def
        COMMAND native-spec2def -n -r ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file} ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_exp.def
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file})

    # Assemble the file
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.obj
        COMMAND ${CMAKE_ASM_COMPILER} /NOLOGO /Fo${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.obj /c /Ta ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.asm
        DEPENDS "${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.asm"
    )

    # Add neccessary importlibs for redirections
    set(_libraries "")
    foreach(_lib ${ARGN})
        list(APPEND _libraries "${CMAKE_BINARY_DIR}/importlibs/${_lib}.lib")
    endforeach()

    # Build the importlib
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/importlibs/lib${_name}.lib
        COMMAND LINK /LIB /NOLOGO /MACHINE:X86 /DEF:${CMAKE_BINARY_DIR}/importlibs/lib${_name}_exp.def /OUT:${CMAKE_BINARY_DIR}/importlibs/lib${_name}.lib ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.obj ${_libraries}
        DEPENDS ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_stubs.obj ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_exp.def ${_libraries}
    )

    # Add the importlib target
    add_custom_target(
        lib${_name}
        DEPENDS ${CMAKE_BINARY_DIR}/importlibs/lib${_name}.lib
    )
endmacro()

macro(add_importlibs MODULE)
    foreach(LIB ${ARGN})
        target_link_libraries(${MODULE} ${CMAKE_BINARY_DIR}/importlibs/lib${LIB}.lib)
        add_dependencies(${MODULE} lib${LIB})
    endforeach()
endmacro()

macro(spec2def _dllname _spec_file)
    get_filename_component(_file ${_spec_file} NAME_WE)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_file}.def
        COMMAND native-spec2def -n --dll ${_dllname} ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file} ${CMAKE_CURRENT_BINARY_DIR}/${_file}.def
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file})
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${_file}.def
        PROPERTIES GENERATED TRUE)
endmacro()

# Optional 3rd parameter: dllname
macro(set_export_spec _module _spec_file)
    get_filename_component(_file ${_spec_file} NAME_WE)
    if (${ARGC} GREATER 2)
        set(_dllname ${ARGV2})
    else()
        set(_dllname ${_file}.dll)
    endif()
    spec2def(${_dllname} ${_spec_file})
endmacro()

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/importlibs)

#pseh workaround
set(PSEH_LIB "")

endif()
