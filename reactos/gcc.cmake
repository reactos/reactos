
# Linking
if(ARCH MATCHES i386)
link_directories("${REACTOS_SOURCE_DIR}/importlibs")
endif()
link_directories(${REACTOS_BINARY_DIR}/lib/sdk/crt)
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_EXE_LINKER_FLAGS "-nodefaultlibs -nostdlib -Wl,--enable-auto-image-base -Wl,--disable-auto-import")
# -Wl,-T,${REACTOS_SOURCE_DIR}/global.lds
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS_INIT} -Wl,--disable-stdcall-fixup")

#set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -i <SOURCE> <CMAKE_C_LINK_FLAGS> <DEFINES> -I${REACTOS_SOURCE_DIR}/include/psdk -I${REACTOS_BINARY_DIR}/include/psdk -I${REACTOS_SOURCE_DIR}/include/ -I${REACTOS_SOURCE_DIR}/include/reactos -I${REACTOS_BINARY_DIR}/include/reactos -I${REACTOS_SOURCE_DIR}/include/reactos/wine -I${REACTOS_SOURCE_DIR}/include/crt -I${REACTOS_SOURCE_DIR}/include/crt/mingw32 -O coff -o <OBJECT>")

# Temporary, until windres issues are fixed
get_target_property(WRC native-wrc IMPORTED_LOCATION_NOCONFIG)
set(CMAKE_RC_COMPILE_OBJECT
    "<CMAKE_C_COMPILER> -DRC_INVOKED -D__WIN32__=1 -D__FLAT__=1 <DEFINES> -I${REACTOS_SOURCE_DIR}/include/psdk -I${REACTOS_BINARY_DIR}/include/psdk -I${REACTOS_SOURCE_DIR}/include/ -I${REACTOS_SOURCE_DIR}/include/reactos -I${REACTOS_BINARY_DIR}/include/reactos -I${REACTOS_SOURCE_DIR}/include/reactos/wine -I${REACTOS_SOURCE_DIR}/include/crt -I${REACTOS_SOURCE_DIR}/include/crt/mingw32 -xc -E <SOURCE> -o <OBJECT>"
    "${WRC} -i <OBJECT> -o <OBJECT>"
    "<CMAKE_RC_COMPILER> -i <OBJECT> -J res -O coff -o <OBJECT>")

# Compiler Core
add_definitions(-pipe -fms-extensions)

set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
set(CMAKE_CXX_CREATE_SHARED_LIBRARY "<CMAKE_CXX_COMPILER> <CMAKE_SHARED_LIBRARY_CXX_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
set(CMAKE_RC_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

# Debugging (Note: DWARF-4 on 4.5.1 when we ship)
add_definitions(-gdwarf-2 -g2 -femit-struct-debug-detailed=none -feliminate-unused-debug-types)

# Tuning
if(ARCH MATCHES i386)
    add_definitions(-march=${OARCH} -mtune=${TUNE})
else()
    add_definitions(-march=${OARCH})
endif()

# Warnings

add_definitions(-Wall -Wno-char-subscripts -Wpointer-arith -Wno-multichar -Wno-error=uninitialized -Wno-unused-value -Winvalid-pch)

if(ARCH MATCHES amd64)
    add_definitions(-Wno-format)
elseif(ARCH MATCHES arm)
    add_definitions(-Wno-attributes)
endif()

# Optimizations

if(OPTIMIZE STREQUAL "1")
    add_definitions(-Os)
elseif(OPTIMIZE STREQUAL "2")
    add_definitions(-Os)
elseif(OPTIMIZE STREQUAL "3")
    add_definitions(-O1)
elseif(OPTIMIZE STREQUAL "4")
    add_definitions(-O2)
elseif(OPTIMIZE STREQUAL "5")
    add_definitions(-O3)
endif()

add_definitions(-fno-strict-aliasing)

if(ARCH MATCHES i386)
    add_definitions(-mpreferred-stack-boundary=2 -fno-set-stack-executable -fno-optimize-sibling-calls)
    if(OPTIMIZE STREQUAL "1")
        add_definitions(-ftracer -momit-leaf-frame-pointer)
    endif()
elseif(ARCH MATCHES amd64)
    add_definitions(-mpreferred-stack-boundary=4)
    if(OPTIMIZE STREQUAL "1")
        add_definitions(-ftracer -momit-leaf-frame-pointer)
    endif()
elseif(ARCH MATCHES arm)
    if(OPTIMIZE STREQUAL "1")
        add_definitions(-ftracer)
    endif()
endif()

# Other
if(ARCH MATCHES amd64)
    add_definitions(-U_X86_ -UWIN32)
elseif(ARCH MATCHES arm)
    add_definitions(-U_UNICODE -UUNICODE)
    add_definitions(-D__MSVCRT__) # DUBIOUS
endif()

# alternative arch name
if(ARCH MATCHES amd64)
    set(ARCH2 x86_64)
else()
    set(ARCH2 ${ARCH})
endif()

macro(add_linkerflag MODULE _flag)
    set(NEW_LINKER_FLAGS ${_flag})
    get_target_property(LINKER_FLAGS ${MODULE} LINK_FLAGS)
    if(LINKER_FLAGS)
        set(NEW_LINKER_FLAGS "${LINKER_FLAGS} ${NEW_LINKER_FLAGS}")
    endif()
    set_target_properties(${MODULE} PROPERTIES LINK_FLAGS ${NEW_LINKER_FLAGS})
endmacro()

# Optional 3rd parameter: stdcall stack bytes
macro(set_entrypoint MODULE ENTRYPOINT)
    if(${ENTRYPOINT} STREQUAL "0")
        add_linkerflag(${MODULE} "-Wl,-entry,0")
    elseif(ARCH MATCHES i386)
        set(_entrysymbol _${ENTRYPOINT})
        if (${ARGC} GREATER 2)
            set(_entrysymbol ${_entrysymbol}@${ARGV2})
        endif()
        add_linkerflag(${MODULE} "-Wl,-entry,${_entrysymbol}")
    else()
        add_linkerflag(${MODULE} "-Wl,-entry,${ENTRYPOINT}")
    endif()
endmacro()

macro(set_subsystem MODULE SUBSYSTEM)
    add_linkerflag(${MODULE} "-Wl,--subsystem,${SUBSYSTEM}")
endmacro()

macro(set_image_base MODULE IMAGE_BASE)
    add_linkerflag(${MODULE} "-Wl,--image-base,${IMAGE_BASE}")
endmacro()

macro(set_module_type MODULE TYPE)

    add_dependencies(${MODULE} psdk)
    if(${IS_CPP})
        target_link_libraries(${MODULE} stlport -lsupc++ -lgcc)
    endif()

    if(${TYPE} MATCHES nativecui)
        set_subsystem(${MODULE} native)
        set_entrypoint(${MODULE} NtProcessStartup 4)
    elseif(${TYPE} MATCHES win32gui)
        set_subsystem(${MODULE} windows)
        if(IS_UNICODE)
            set_entrypoint(${MODULE} wWinMainCRTStartup)
        else()
            set_entrypoint(${MODULE} WinMainCRTStartup)
        endif(IS_UNICODE)
    elseif(${TYPE} MATCHES win32cui)
        set_subsystem(${MODULE} console)
        if(IS_UNICODE)
            set_entrypoint(${MODULE} wmainCRTStartup)
        else()
            set_entrypoint(${MODULE} mainCRTStartup)
        endif(IS_UNICODE)
    elseif(${TYPE} MATCHES win32dll)
		set_entrypoint(${MODULE} DllMain 12)
        if(DEFINED baseaddress_${MODULE})
            set_image_base(${MODULE} ${baseaddress_${MODULE}})
        else()
            message(STATUS "${MODULE} has no base address")
        endif()
    elseif(${TYPE} MATCHES win32ocx)
        set_entrypoint(${MODULE} DllMain 12)
        set_target_properties(${MODULE} PROPERTIES SUFFIX ".ocx")
    elseif(${TYPE} MATCHES cpl)
        set_entrypoint(${MODULE} DllMain 12)
        set_target_properties(${MODULE} PROPERTIES SUFFIX ".cpl")
    elseif(${TYPE} MATCHES kernelmodedriver)
        set_target_properties(${MODULE} PROPERTIES LINK_FLAGS "-Wl,--exclude-all-symbols -Wl,-file-alignment=0x1000 -Wl,-section-alignment=0x1000" SUFFIX ".sys")
        set_entrypoint(${MODULE} DriverEntry 8)
        set_subsystem(${MODULE} native)
        set_image_base(${MODULE} 0x00010000)
        add_dependencies(${MODULE} bugcodes)
    elseif(${TYPE} MATCHES nativedll)
        set_subsystem(${MODULE} native)
    else()
        message(FATAL_ERROR "Unknown module type : ${TYPE}")
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
    endforeach()

    foreach(arg ${includes})
        set(rc_result_incs "-I${arg} ${rc_result_incs}")
    endforeach()

    #set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> ${rc_result_defs} ${rc_result_incs} -i <SOURCE> -O coff -o <OBJECT>")
    set(CMAKE_RC_COMPILE_OBJECT
    "<CMAKE_C_COMPILER> -DRC_INVOKED -D__WIN32__=1 -D__FLAT__=1 ${rc_result_defs} -I${CMAKE_CURRENT_SOURCE_DIR} ${rc_result_incs} -xc -E <SOURCE> -o <OBJECT>"
    "${WRC} -I${CMAKE_CURRENT_SOURCE_DIR} -i <OBJECT> -o <OBJECT>"
    "<CMAKE_RC_COMPILER> -i <OBJECT> -J res -O coff -o <OBJECT>")
endmacro()

#idl files support
set(IDL_COMPILER native-widl)

if(ARCH MATCHES i386)
    set(IDL_FLAGS -m32 --win32)
elseif(ARCH MATCHES amd64)
    set(IDL_FLAGS -m64 --win64)
endif()

set(IDL_HEADER_ARG -h -o) #.h
set(IDL_TYPELIB_ARG -t -o) #.tlb
set(IDL_SERVER_ARG -s -S) #.c for server library
set(IDL_CLIENT_ARG -c -C) #.c for stub client library
set(IDL_PROXY_ARG -p -P)
set(IDL_INTERFACE_ARG -u -o)
set(IDL_DLLDATA_ARG --dlldata-only -o)


macro(add_importlibs MODULE)
    add_dependency_node(${MODULE})
    foreach(LIB ${ARGN})
        if ("${LIB}" MATCHES "msvcrt")
            target_link_libraries(${MODULE} msvcrtex)
        endif()
        target_link_libraries(${MODULE} ${CMAKE_BINARY_DIR}/importlibs/lib${LIB}.a)
        add_dependencies(${MODULE} lib${LIB})
        add_dependency_edge(${MODULE} ${LIB})
    endforeach()
endmacro()

macro(add_delay_importlibs MODULE)
    foreach(LIB ${ARGN})
        target_link_libraries(${MODULE} ${CMAKE_BINARY_DIR}/importlibs/lib${LIB}_delayed.a)
        add_dependencies(${MODULE} lib${LIB}_delayed)
    endforeach()
    target_link_libraries(${MODULE} delayimp)
endmacro()

if(NOT ARCH MATCHES i386)
    set(DECO_OPTION "-@")
endif()

macro(add_importlib_target _exports_file)

    get_filename_component(_name ${_exports_file} NAME_WE)
    get_filename_component(_extension ${_exports_file} EXT)
    get_target_property(_suffix ${_name} SUFFIX)
    if(${_suffix} STREQUAL "_suffix-NOTFOUND")
        get_target_property(_type ${_name} TYPE)
        if(${_type} MATCHES EXECUTABLE)
            set(_suffix ".exe")
        else()
            set(_suffix ".dll")
        endif()
    endif()

    if (${_extension} STREQUAL ".spec")

        # Normal importlib creation
        add_custom_command(
            OUTPUT ${CMAKE_BINARY_DIR}/importlibs/lib${_name}.a
            COMMAND native-spec2def -n=${_name}${_suffix} -a=${ARCH2} -d=${CMAKE_CURRENT_BINARY_DIR}/${_name}_implib.def ${CMAKE_CURRENT_SOURCE_DIR}/${_exports_file}
            COMMAND ${MINGW_PREFIX}dlltool --def ${CMAKE_CURRENT_BINARY_DIR}/${_name}_implib.def --kill-at --output-lib=${CMAKE_BINARY_DIR}/importlibs/lib${_name}.a
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_exports_file})

        # Delayed importlib creation
        add_custom_command(
            OUTPUT ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_delayed.a
            COMMAND native-spec2def -n=${_name}${_suffix} -a=${ARCH2} -d=${CMAKE_CURRENT_BINARY_DIR}/${_name}_delayed_implib.def ${CMAKE_CURRENT_SOURCE_DIR}/${_exports_file}
            COMMAND ${MINGW_PREFIX}dlltool --def ${CMAKE_CURRENT_BINARY_DIR}/${_name}_delayed_implib.def --kill-at --output-delaylib ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_delayed.a
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_exports_file})

    elseif(${_extension} STREQUAL ".def")
        message("Use of def files for import libs is deprecated: ${_exports_file}")
        add_custom_command(
            OUTPUT ${CMAKE_BINARY_DIR}/importlibs/lib${_name}.a
            COMMAND ${MINGW_PREFIX}dlltool --def ${CMAKE_CURRENT_SOURCE_DIR}/${_exports_file} --kill-at --output-lib=${CMAKE_BINARY_DIR}/importlibs/lib${_name}.a
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_exports_file})
        add_custom_command(
            OUTPUT ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_delayed.a
            COMMAND ${MINGW_PREFIX}dlltool --def ${CMAKE_CURRENT_SOURCE_DIR}/${_exports_file} --kill-at --output-delaylib ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_delayed.a
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_exports_file})
    else()
        message(FATAL_ERROR "Unsupported exports file extension: ${_extension}")
    endif()

    # Normal importlib target
    add_custom_target(
        lib${_name}
        DEPENDS ${CMAKE_BINARY_DIR}/importlibs/lib${_name}.a)
    # Delayed importlib target
    add_custom_target(
        lib${_name}_delayed
        DEPENDS ${CMAKE_BINARY_DIR}/importlibs/lib${_name}_delayed.a)

endmacro()

macro(spec2def _dllname _spec_file)
    get_filename_component(_file ${_spec_file} NAME_WE)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_file}.def ${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c
        COMMAND native-spec2def -n=${_dllname} --kill-at -a=${ARCH2} -d=${CMAKE_CURRENT_BINARY_DIR}/${_file}.def -s=${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file})
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${_file}.def
        PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE)
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c PROPERTIES GENERATED TRUE)
endmacro()

macro(macro_mc FILE)
    set(COMMAND_MC ${MINGW_PREFIX}windmc -A -b ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.mc -r ${REACTOS_BINARY_DIR}/include/reactos -h ${REACTOS_BINARY_DIR}/include/reactos)
endmacro()

#pseh lib, needed with mingw
set(PSEH_LIB "pseh")

# Macros
macro(_PCH_GET_COMPILE_FLAGS _target_name _out_compile_flags _header_filename)
    # Add the precompiled header to the build
    get_filename_component(_FILE ${_header_filename} NAME)
    set(_gch_filename "${_FILE}.gch")
    list(APPEND ${_out_compile_flags} -c ${_header_filename} -o ${_gch_filename})

    # This gets us our includes
    get_directory_property(DIRINC INCLUDE_DIRECTORIES)
    foreach(item ${DIRINC})
        list(APPEND ${_out_compile_flags} -I${item})
    endforeach()

    # This our definitions
    get_directory_property(_compiler_flags DEFINITIONS)
    list(APPEND ${_out_compile_flags} ${_compiler_flags})

    # This gets any specific definitions that were added with set-target-property
    get_target_property(_target_defs ${_target_name} COMPILE_DEFINITIONS)
    if (_target_defs)
        foreach(item ${_target_defs})
            list(APPEND ${_out_compile_flags} -D${item})
        endforeach()
    endif()

	separate_arguments(${_out_compile_flags})
endmacro()

macro(add_pch _target_name _FILE)
	#set(_header_filename ${CMAKE_CURRENT_SOURCE_DIR}/${_FILE})
	#get_filename_component(_basename ${_FILE} NAME)
    #set(_gch_filename ${_basename}.gch)
    #_PCH_GET_COMPILE_FLAGS(${_target_name} _args ${_header_filename})

    #add_custom_command(OUTPUT ${_gch_filename} COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_COMPILER_ARG1} ${_args} DEPENDS ${_header_filename})
	#get_target_property(_src_files ${_target_name} SOURCES)
	#set_source_files_properties(${_src_files} PROPERTIES COMPILE_FLAGS "-Winvalid-pch -fpch-preprocess" #OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_gch_filename})
	#add_linkerflag(${_target_name} "${_gch_filename}")
endmacro()

macro(CreateBootSectorTarget _target_name _asm_file _object_file _base_address)
    get_filename_component(OBJECT_PATH ${_object_file} PATH)
    get_filename_component(OBJECT_NAME ${_object_file} NAME)
    file(MAKE_DIRECTORY ${OBJECT_PATH})
    get_directory_property(defines COMPILE_DEFINITIONS)
    get_directory_property(includes INCLUDE_DIRECTORIES)

    foreach(arg ${defines})
        set(result_defs ${result_defs} -D${arg})
    endforeach()

    foreach(arg ${includes})
        set(result_incs -I${arg} ${result_incs})
    endforeach()

    add_custom_command(
        OUTPUT ${_object_file}
        COMMAND nasm -o ${_object_file} ${result_incs} ${result_defs} -f bin ${_asm_file}
        DEPENDS ${_asm_file})
    set_source_files_properties(${_object_file} PROPERTIES GENERATED TRUE)
    add_custom_target(${_target_name} ALL DEPENDS ${_object_file})
endmacro()

macro(CreateBootSectorTarget2 _target_name _asm_file _binary_file _base_address)
    set(_object_file ${_binary_file}.o)

    add_custom_command(
        OUTPUT ${_object_file}
        COMMAND ${CMAKE_ASM_COMPILER} -x assembler-with-cpp -o ${_object_file} -I${REACTOS_SOURCE_DIR}/include/asm -I${REACTOS_BINARY_DIR}/include/asm -D__ASM__ -c ${_asm_file}
        DEPENDS ${_asm_file})

    add_custom_command(
        OUTPUT ${_binary_file}
        COMMAND native-obj2bin ${_object_file} ${_binary_file} ${_base_address}
        # COMMAND objcopy --output-target binary --image-base 0x${_base_address} ${_object_file} ${_binary_file}
        DEPENDS ${_object_file})

    set_source_files_properties(${_object_file} ${_binary_file} PROPERTIES GENERATED TRUE)

    add_custom_target(${_target_name} ALL DEPENDS ${_binary_file})

endmacro()
