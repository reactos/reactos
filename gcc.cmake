
if(NOT CMAKE_CROSSCOMPILING)

add_definitions(-fshort-wchar)

else()

# Linking
link_directories("${REACTOS_SOURCE_DIR}/importlibs" ${REACTOS_BINARY_DIR}/lib/3rdparty/mingw)
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_EXE_LINKER_FLAGS "-nodefaultlibs -nostdlib -Wl,--enable-auto-image-base -Wl,--kill-at -Wl,--disable-auto-import")
# -Wl,-T,${REACTOS_SOURCE_DIR}/global.lds

# Compiler Core
add_definitions(-pipe -fms-extensions)

set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
set(CMAKE_CXX_CREATE_SHARED_LIBRARY "<CMAKE_CXX_COMPILER> <CMAKE_SHARED_LIBRARY_CXX_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
set(CMAKE_RC_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

# Debugging (Note: DWARF-4 on 4.5.1 when we ship)
#add_definitions(-gdwarf-2 -g2 -femit-struct-debug-detailed=none -feliminate-unused-debug-types)

# Tuning
add_definitions(-march=pentium -mtune=i686)

# Warnings
add_definitions(-Wall -Wno-char-subscripts -Wpointer-arith -Wno-multichar -Wno-error=uninitialized -Wno-unused-value -Winvalid-pch)

# Optimizations
add_definitions(-Os -fno-strict-aliasing -ftracer -momit-leaf-frame-pointer -mpreferred-stack-boundary=2 -fno-set-stack-executable -fno-optimize-sibling-calls)

# Macros
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
        add_linkerflag(${MODULE} "-Wl,-entry,0")
    else()
        add_linkerflag(${MODULE} "-Wl,-entry,_${ENTRYPOINT}")
    endif()
endmacro()

macro(set_subsystem MODULE SUBSYSTEM)
    add_linkerflag(${MODULE} "-Wl,--subsystem,${SUBSYSTEM}")
endmacro()

macro(set_image_base MODULE IMAGE_BASE)
    set(NEW_LINKER_FLAGS "-Wl,--image-base,${IMAGE_BASE}")
    get_target_property(LINKER_FLAGS ${MODULE} LINK_FLAGS)
    if(LINKER_FLAGS)
        set(NEW_LINKER_FLAGS "${LINKER_FLAGS} ${NEW_LINKER_FLAGS}")
    endif()
    set_target_properties(${MODULE} PROPERTIES LINK_FLAGS ${NEW_LINKER_FLAGS})
endmacro()

macro(add_importlibs MODULE)
  foreach(LIB ${ARGN})
    target_link_libraries(${MODULE} ${LIB}.dll.a)
  endforeach()
endmacro()

macro(set_module_type MODULE TYPE)

    add_dependencies(${MODULE} psdk buildno_header)
	
    if(${IS_CPP})
	  target_link_libraries(${MODULE} stlport -lsupc++ -lgcc)
	endif()
	
    if(${TYPE} MATCHES nativecui)
        set_subsystem(${MODULE} native)
        set_entrypoint(${MODULE} NtProcessStartup@4)
    endif()
    if(${TYPE} MATCHES win32gui)
        set_subsystem(${MODULE} windows)
        set_entrypoint(${MODULE} WinMainCRTStartup)
        if(NOT IS_UNICODE)
            target_link_libraries(${MODULE} mingw_main)
        else()
            target_link_libraries(${MODULE} mingw_wmain)
        endif(NOT IS_UNICODE)
		target_link_libraries(${MODULE} mingw_common)
    endif()
    if(${TYPE} MATCHES win32cui)
        set_subsystem(${MODULE} console)
        set_entrypoint(${MODULE} mainCRTStartup)
        if(NOT IS_UNICODE)
            target_link_libraries(${MODULE} mingw_main)
        else()
            target_link_libraries(${MODULE} mingw_wmain)
        endif(NOT IS_UNICODE)
		target_link_libraries(${MODULE} mingw_common)
    endif()
    if(${TYPE} MATCHES win32dll)
        set_entrypoint(${MODULE} DllMain@12)
		if(DEFINED baseaddress_${MODULE})
			set_image_base(${MODULE} ${baseaddress_${MODULE}})
		else()
			message(STATUS "${MODULE} has no base address")
		endif()
    endif()
    if(${TYPE} MATCHES win32ocx)
        set_entrypoint(${MODULE} DllMain@12)
        set_target_properties(${MODULE} PROPERTIES SUFFIX ".ocx")
    endif()
    if(${TYPE} MATCHES cpl)
        set_entrypoint(${MODULE} DllMain@12)
        set_target_properties(${MODULE} PROPERTIES SUFFIX ".cpl")
    endif()
	if(${TYPE} MATCHES kernelmodedriver)
	    set_target_properties(${MODULE} PROPERTIES LINK_FLAGS "-Wl,--exclude-all-symbols" SUFFIX ".sys")
	    set_entrypoint(${MODULE} DriverEntry@8)
		set_subsystem(${MODULE} native)
        set_image_base(${MODULE} 0x00010000)
		add_dependencies(${MODULE} bugcodes)
	endif()
endmacro()

macro(set_unicode)
   add_definitions(-DUNICODE -D_UNICODE)
   set(IS_UNICODE 1)
endmacro()

# Workaround lack of mingw RC support in cmake
macro(set_rc_compiler)
    get_directory_property(defines COMPILE_DEFINITIONS)
    get_directory_property(includes INCLUDE_DIRECTORIES)

    foreach(arg ${defines})
        set(result_defs "${result_defs} -D${arg}")
    endforeach(arg ${defines})

    foreach(arg ${includes})
        set(result_incs "-I${arg} ${result_incs}")
    endforeach(arg ${includes})

    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> ${result_defs} ${result_incs} -i <SOURCE> -O coff -o <OBJECT>")
endmacro()

#idl files support
set(IDL_COMPILER native-widl)
set(IDL_FLAGS -m32 --win32)
set(IDL_HEADER_ARG -h -H) #.h
set(IDL_TYPELIB_ARG -t -T) #.tlb
set(IDL_SERVER_ARG -s -S) #.c for server library
set(IDL_CLIENT_ARG -c -C) #.c for stub client library

macro(add_importlib_target _def_file)
  # empty for now, while import libs are shipped
endmacro()

macro(pdef2def _pdef_file)
    get_filename_component(_file ${_pdef_file} NAME_WE)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_file}.def
        COMMAND ${MINGW_PREFIX}cpp -o ${CMAKE_CURRENT_BINARY_DIR}/${_file}.def -P -E ${CMAKE_CURRENT_SOURCE_DIR}/${_pdef_file}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_pdef_file})
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${_file}.def
        PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE)
    add_custom_target(
        ${_file}_def
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_file}.def)
endmacro(pdef2def _pdef_file)

macro(set_pdef_file _module _pdef_file)
    pdef2def(${_pdef_file})
    get_filename_component(_file ${_pdef_file} NAME_WE)
    target_link_libraries(${_module} "${CMAKE_CURRENT_BINARY_DIR}/${_file}.def")
    add_dependencies(${_module} ${_file}_def)
endmacro()

#pseh lib, needed with mingw
set(PSEH_LIB "pseh")

endif()
