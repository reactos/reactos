
if(NOT CMAKE_CROSSCOMPILING)

add_definitions(-fshort-wchar)

else()

# Linking
link_directories("${REACTOS_SOURCE_DIR}/importlibs" ${REACTOS_BINARY_DIR}/lib/3rdparty/mingw)
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_EXE_LINKER_FLAGS "-nodefaultlibs -nostdlib -Wl,--enable-auto-image-base -Wl,--kill-at -Wl,--disable-auto-import")
# -Wl,-T,${REACTOS_SOURCE_DIR}/global.lds
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS_INIT} -Wl,--disable-stdcall-fixup")

# Compiler Core
add_definitions(-pipe -fms-extensions)

set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
set(CMAKE_CXX_CREATE_SHARED_LIBRARY "<CMAKE_CXX_COMPILER> <CMAKE_SHARED_LIBRARY_CXX_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
set(CMAKE_RC_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

# Debugging (Note: DWARF-4 on 4.5.1 when we ship)
add_definitions(-gdwarf-2 -g2 -femit-struct-debug-detailed=none -feliminate-unused-debug-types)

# Tuning
add_definitions(-march=pentium -mtune=i686)

# Warnings
add_definitions(-Wall -Wno-char-subscripts -Wpointer-arith -Wno-multichar -Wno-error=uninitialized -Wno-unused-value -Winvalid-pch)

# Optimizations
add_definitions(-Os -fno-strict-aliasing -ftracer -momit-leaf-frame-pointer -mpreferred-stack-boundary=2 -fno-set-stack-executable -fno-optimize-sibling-calls)

# Macros
MACRO(_PCH_GET_COMPILE_FLAGS _target_name _out_compile_flags _header_filename)
    # Add the precompiled header to the build
    get_filename_component(FILE ${_header_filename} NAME)
    set(_gch_filename "${_target_name}_${FILE}.gch")
    list(APPEND ${_out_compile_flags} -c ${_header_filename} -o ${_gch_filename})

    # This gets us our includes
    get_directory_property(DIRINC INCLUDE_DIRECTORIES)
    foreach(item ${DIRINC})
        list(APPEND ${_out_compile_flags} -I${item})
    endforeach(item) 

    # This is a particular bit of undocumented/hacky magic I'm quite proud of
    get_directory_property(_compiler_flags DEFINITIONS)
    string(REPLACE "\ " "\t" _compiler_flags ${_compiler_flags})
    list(APPEND ${_out_compile_flags} ${_compiler_flags})

    # This gets any specific definitions that were added with set-target-property
    get_target_property(_target_defs ${_target_name} COMPILE_DEFINITIONS)
    if (_target_defs)
        foreach(item ${_target_defs})
            list(APPEND ${_out_compile_flags} -D${item})
        endforeach(item)
    endif()
ENDMACRO(_PCH_GET_COMPILE_FLAGS) 

MACRO(add_pch _target_name _header_filename _src_list)
    get_filename_component(FILE ${_header_filename} NAME)
    set(_gch_filename "${_target_name}_${FILE}.gch")
    list(APPEND ${_src_list} ${_gch_filename})
    _PCH_GET_COMPILE_FLAGS(${_target_name} _args ${_header_filename})
    file(REMOVE ${_gch_filename})
    add_custom_command(
        OUTPUT ${_gch_filename}
        COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_COMPILER_ARG1} ${_args}
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
        add_linkerflag(${MODULE} "-Wl,-entry,0")
    else()
        add_linkerflag(${MODULE} "-Wl,-entry,_${ENTRYPOINT}")
    endif()
endmacro()

macro(set_subsystem MODULE SUBSYSTEM)
    add_linkerflag(${MODULE} "-Wl,--subsystem,${SUBSYSTEM}")
endmacro()

macro(set_image_base MODULE IMAGE_BASE)
    add_linkerflag(${MODULE} "-Wl,--image-base,${IMAGE_BASE}")
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
        set_entrypoint(${MODULE} DllMainCRTStartup@12)
        target_link_libraries(${MODULE} mingw_dllmain mingw_common)
		if(DEFINED baseaddress_${MODULE})
			set_image_base(${MODULE} ${baseaddress_${MODULE}})
		else()
			message(STATUS "${MODULE} has no base address")
		endif()
    endif()
    if(${TYPE} MATCHES win32ocx)
        set_entrypoint(${MODULE} DllMainCRTStartup@12)
        target_link_libraries(${MODULE} mingw_dllmain mingw_common)
        set_target_properties(${MODULE} PROPERTIES SUFFIX ".ocx")
    endif()
    if(${TYPE} MATCHES cpl)
        set_entrypoint(${MODULE} DllMainCRTStartup@12)
        target_link_libraries(${MODULE} mingw_dllmain mingw_common)
        set_target_properties(${MODULE} PROPERTIES SUFFIX ".cpl")
    endif()
	if(${TYPE} MATCHES kernelmodedriver)
	    set_target_properties(${MODULE} PROPERTIES LINK_FLAGS "-Wl,--exclude-all-symbols -Wl,-file-alignment=0x1000 -Wl,-section-alignment=0x1000" SUFFIX ".sys")
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
        set(rc_result_defs "${rc_result_defs} -D${arg}")
    endforeach(arg ${defines})

    foreach(arg ${includes})
        set(rc_result_incs "-I${arg} ${rc_result_incs}")
    endforeach(arg ${includes})

    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> ${rc_result_defs} ${rc_result_incs} -i <SOURCE> -O coff -o <OBJECT>")
endmacro()

#idl files support
set(IDL_COMPILER native-widl)
set(IDL_FLAGS -m32 --win32)
set(IDL_HEADER_ARG -h -H) #.h
set(IDL_TYPELIB_ARG -t -T) #.tlb
set(IDL_SERVER_ARG -s -S) #.c for server library
set(IDL_CLIENT_ARG -c -C) #.c for stub client library
set(IDL_PROXY_ARG -p -P)
set(IDL_DLLDATA_ARG --dlldata-only --dlldata=)

macro(add_importlibs MODULE)
    foreach(LIB ${ARGN})
        target_link_libraries(${MODULE} ${CMAKE_BINARY_DIR}/importlibs/lib${LIB}.a)
        add_dependencies(${MODULE} lib${LIB})
    endforeach()
endmacro()

macro(add_importlib_target _spec_file)
    get_filename_component(_name ${_spec_file} NAME_WE)
    
    if (${ARGC} GREATER 1)
        set(DLLNAME_OPTION "-n=${ARGV1}")
    else()
        set(DLLNAME_OPTION "")
    endif()
    
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/importlibs/lib${_name}.a
        COMMAND native-spec2def ${DLLNAME_OPTION} -d=${CMAKE_CURRENT_BINARY_DIR}/${_name}.def ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file}
        COMMAND ${MINGW_PREFIX}dlltool --def ${CMAKE_CURRENT_BINARY_DIR}/${_name}.def --kill-at --output-lib=${CMAKE_BINARY_DIR}/importlibs/lib${_name}.a
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file})
    add_custom_target(
        lib${_name}
        DEPENDS ${CMAKE_BINARY_DIR}/importlibs/lib${_name}.a)
endmacro()

macro(spec2def _dllname _spec_file)
    get_filename_component(_file ${_spec_file} NAME_WE)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_file}.def ${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c
        COMMAND native-spec2def -n=${_dllname} -d=${CMAKE_CURRENT_BINARY_DIR}/${_file}.def -s=${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file})
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${_file}.def
        PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE)
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c PROPERTIES GENERATED TRUE)
    list(APPEND SOURCE ${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c)
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

#pseh lib, needed with mingw
set(PSEH_LIB "pseh")

endif()
