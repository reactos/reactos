
# Show a note about ccache build
if(CCACHE STREQUAL "ccache")
    message("-- Enabling ccache build - done")
endif()

# PDB style debug info
if(NOT DEFINED SEPARATE_DBG)
    set(SEPARATE_DBG FALSE)
endif()

# Compiler Core
add_compile_flags("-pipe -fms-extensions")

#bug
#file(TO_NATIVE_PATH ${REACTOS_SOURCE_DIR} REACTOS_SOURCE_DIR_NATIVE)
#workaround
set(REACTOS_SOURCE_DIR_NATIVE ${REACTOS_SOURCE_DIR})
 if(CMAKE_HOST_SYSTEM_NAME MATCHES Windows)
string(REPLACE "/" "\\" REACTOS_SOURCE_DIR_NATIVE ${REACTOS_SOURCE_DIR})
endif()
add_compile_flags("-fdebug-prefix-map=${REACTOS_SOURCE_DIR_NATIVE}=ReactOS")

# Debugging
if(SEPARATE_DBG)
    add_compile_flags("-gdwarf-2 -g2")
else()
    add_compile_flags("-gstabs+")
endif()

# Do not allow warnings
add_compile_flags("-Werror")

# For some reason, cmake sets -fPIC, and we don't want it
if(DEFINED CMAKE_SHARED_LIBRARY_ASM_FLAGS)
    string(REPLACE "-fPIC" "" CMAKE_SHARED_LIBRARY_ASM_FLAGS ${CMAKE_SHARED_LIBRARY_ASM_FLAGS})
endif()

# Tuning
if(ARCH MATCHES i386)
    add_compile_flags("-march=${OARCH} -mtune=${TUNE}")
else()
    add_compile_flags("-march=${OARCH}")
endif()

# Warnings
add_compile_flags("-Wall -Wno-char-subscripts -Wpointer-arith -Wno-multichar -Wno-unused-value")

if(GCC_VERSION VERSION_LESS 4.6)
    add_compile_flags("-Wno-error=uninitialized")
elseif((GCC_VERSION VERSION_GREATER 4.6 OR GCC_VERSION VERSION_EQUAL 4.6) AND GCC_VERSION VERSION_LESS 4.7)
    add_compile_flags("-Wno-error=unused-but-set-variable -Wno-error=uninitialized")
elseif(GCC_VERSION VERSION_EQUAL 4.7 OR GCC_VERSION VERSION_GREATER 4.7)
    add_compile_flags("-Wno-error=unused-but-set-variable -Wno-error=maybe-uninitialized -Wno-error=delete-non-virtual-dtor -Wno-error=narrowing")
endif()

if(ARCH MATCHES amd64)
    add_compile_flags("-Wno-format")
elseif(ARCH MATCHES arm)
    add_compile_flags("-Wno-attributes")
endif()

# Optimizations
if(OPTIMIZE STREQUAL "1")
    add_compile_flags("-Os")
elseif(OPTIMIZE STREQUAL "2")
    add_compile_flags("-Os")
elseif(OPTIMIZE STREQUAL "3")
    add_compile_flags("-O1")
elseif(OPTIMIZE STREQUAL "4")
    add_compile_flags("-O2")
elseif(OPTIMIZE STREQUAL "5")
    add_compile_flags("-O3")
endif()

# Link-time code generation
if(LTCG)
    add_compile_flags("-flto -Wno-error=clobbered")
endif()

add_compile_flags("-fno-strict-aliasing")

if(ARCH MATCHES i386)
    add_compile_flags("-mpreferred-stack-boundary=3 -fno-set-stack-executable -fno-optimize-sibling-calls -fno-omit-frame-pointer")
    if(OPTIMIZE STREQUAL "1")
        add_compile_flags("-ftracer -momit-leaf-frame-pointer")
    endif()
elseif(ARCH MATCHES amd64)
    add_compile_flags("-mpreferred-stack-boundary=4")
    if(OPTIMIZE STREQUAL "1")
        add_compile_flags("-ftracer -momit-leaf-frame-pointer")
    endif()
elseif(ARCH MATCHES arm)
    if(OPTIMIZE STREQUAL "1")
        add_compile_flags("-ftracer")
    endif()
endif()

# Other
if(ARCH MATCHES amd64)
    add_definitions(-U_X86_ -UWIN32)
elseif(ARCH MATCHES arm)
    add_definitions(-U_UNICODE -UUNICODE)
    add_definitions(-D__MSVCRT__) # DUBIOUS
endif()

add_definitions(-D_inline=__inline)

# alternative arch name
if(ARCH MATCHES amd64)
    set(ARCH2 x86_64)
else()
    set(ARCH2 ${ARCH})
endif()

if(SEPARATE_DBG)
    # PDB style debug puts all dwarf debug info in a separate dbg file
    message(STATUS "Building separate debug symbols")
    file(MAKE_DIRECTORY ${REACTOS_BINARY_DIR}/symbols)
    if(CMAKE_GENERATOR MATCHES "Ninja")
        set(SYMBOL_FILE <TARGET_PDB>)
    else()
        set(SYMBOL_FILE <TARGET>.gdb)
    endif()
    set(OBJCOPY ${CMAKE_OBJCOPY})
    set(CMAKE_C_LINK_EXECUTABLE
        "<CMAKE_C_COMPILER> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>"
        "${OBJCOPY} --only-keep-debug <TARGET> ${REACTOS_BINARY_DIR}/symbols/${SYMBOL_FILE}"
        "${OBJCOPY} --strip-debug <TARGET>")
    set(CMAKE_CXX_LINK_EXECUTABLE
        "<CMAKE_CXX_COMPILER> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>"
        "${OBJCOPY} --only-keep-debug <TARGET> ${REACTOS_BINARY_DIR}/symbols/${SYMBOL_FILE}"
        "${OBJCOPY} --strip-debug <TARGET>")
    set(CMAKE_C_CREATE_SHARED_LIBRARY
        "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>"
        "${OBJCOPY} --only-keep-debug <TARGET> ${REACTOS_BINARY_DIR}/symbols/${SYMBOL_FILE}"
        "${OBJCOPY} --strip-debug <TARGET>")
    set(CMAKE_CXX_CREATE_SHARED_LIBRARY
        "<CMAKE_CXX_COMPILER> <CMAKE_SHARED_LIBRARY_CXX_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>"
        "${OBJCOPY} --only-keep-debug <TARGET> ${REACTOS_BINARY_DIR}/symbols/${SYMBOL_FILE}"
        "${OBJCOPY} --strip-debug <TARGET>")
    set(CMAKE_RC_CREATE_SHARED_LIBRARY
        "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>"
        "${OBJCOPY} --only-keep-debug <TARGET> ${REACTOS_BINARY_DIR}/symbols/${SYMBOL_FILE}"
        "${OBJCOPY} --strip-debug <TARGET>")
else()
    # Normal rsym build
    get_target_property(RSYM native-rsym IMPORTED_LOCATION_NOCONFIG)
    set(CMAKE_C_LINK_EXECUTABLE
        "<CMAKE_C_COMPILER> ${CMAKE_C_FLAGS} <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>"
        "${RSYM} <TARGET> <TARGET>")
    set(CMAKE_CXX_LINK_EXECUTABLE
        "<CMAKE_CXX_COMPILER> ${CMAKE_CXX_FLAGS} <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>"
        "${RSYM} <TARGET> <TARGET>")
    set(CMAKE_C_CREATE_SHARED_LIBRARY
        "<CMAKE_C_COMPILER> ${CMAKE_C_FLAGS} <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>"
        "${RSYM} <TARGET> <TARGET>")
    set(CMAKE_CXX_CREATE_SHARED_LIBRARY
        "<CMAKE_CXX_COMPILER> ${CMAKE_CXX_FLAGS} <CMAKE_SHARED_LIBRARY_CXX_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>"
        "${RSYM} <TARGET> <TARGET>")
    set(CMAKE_RC_CREATE_SHARED_LIBRARY
        "<CMAKE_C_COMPILER> ${CMAKE_C_FLAGS} <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>"
        "${RSYM} <TARGET> <TARGET>")
endif()

set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -Wl,--enable-auto-image-base,--disable-auto-import,--disable-stdcall-fixup")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS_INIT} -Wl,--disable-stdcall-fixup")

SET(CMAKE_C_COMPILE_OBJECT "${CCACHE} <CMAKE_C_COMPILER> <DEFINES> <FLAGS> -o <OBJECT> -c <SOURCE>")
SET(CMAKE_CXX_COMPILE_OBJECT "${CCACHE} <CMAKE_CXX_COMPILER>  <DEFINES> <FLAGS> -o <OBJECT> -c <SOURCE>")
set(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> -x assembler-with-cpp -o <OBJECT> -I${REACTOS_SOURCE_DIR}/include/asm -I${REACTOS_BINARY_DIR}/include/asm <FLAGS> <DEFINES> -D__ASM__ -c <SOURCE>")

set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff <FLAGS> -DRC_INVOKED -D__WIN32__=1 -D__FLAT__=1 ${I18N_DEFS} <DEFINES> <SOURCE> <OBJECT>")
set(CMAKE_DEPFILE_FLAGS_RC "--preprocessor \"<CMAKE_C_COMPILER> -E -xc-header -MMD -MF <DEPFILE> -MT <OBJECT>\"")

# Optional 3rd parameter: stdcall stack bytes
function(set_entrypoint MODULE ENTRYPOINT)
    if(${ENTRYPOINT} STREQUAL "0")
        add_target_link_flags(${MODULE} "-Wl,-entry,0")
    elseif(ARCH MATCHES i386)
        set(_entrysymbol _${ENTRYPOINT})
        if(${ARGC} GREATER 2)
            set(_entrysymbol ${_entrysymbol}@${ARGV2})
        endif()
        add_target_link_flags(${MODULE} "-Wl,-entry,${_entrysymbol}")
    else()
        add_target_link_flags(${MODULE} "-Wl,-entry,${ENTRYPOINT}")
    endif()
endfunction()

function(set_subsystem MODULE SUBSYSTEM)
    add_target_link_flags(${MODULE} "-Wl,--subsystem,${SUBSYSTEM}")
endfunction()

function(set_image_base MODULE IMAGE_BASE)
    add_target_link_flags(${MODULE} "-Wl,--image-base,${IMAGE_BASE}")
endfunction()

function(set_module_type_toolchain MODULE TYPE)
    if(IS_CPP)
        target_link_libraries(${MODULE} -lstdc++ -lsupc++ -lgcc -lmingwex)
    endif()

    if(${TYPE} STREQUAL kernelmodedriver)
        add_target_link_flags(${MODULE} "-Wl,--exclude-all-symbols,-file-alignment=0x1000,-section-alignment=0x1000")
    endif()
endfunction()

function(set_rc_compiler)
endfunction()

function(add_delay_importlibs MODULE)
    foreach(LIB ${ARGN})
        target_link_libraries(${MODULE} lib${LIB}_delayed)
    endforeach()
    target_link_libraries(${MODULE} delayimp)
endfunction()

if(NOT ARCH MATCHES i386)
    set(DECO_OPTION "-@")
endif()

function(generate_import_lib _libname _dllname _spec_file)
    # generate the def for the import lib
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_libname}_implib.def
        COMMAND native-spec2def -n=${_dllname} -a=${ARCH2} -d=${CMAKE_CURRENT_BINARY_DIR}/${_libname}_implib.def ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file} native-spec2def)
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${_libname}_implib.def PROPERTIES EXTERNAL_OBJECT TRUE)

    #create normal importlib
    _add_library(${_libname} STATIC EXCLUDE_FROM_ALL ${CMAKE_CURRENT_BINARY_DIR}/${_libname}_implib.def)
    set_target_properties(${_libname} PROPERTIES LINKER_LANGUAGE "IMPLIB" PREFIX "")

    #create delayed importlib
    _add_library(${_libname}_delayed STATIC EXCLUDE_FROM_ALL ${CMAKE_CURRENT_BINARY_DIR}/${_libname}_implib.def)
    set_target_properties(${_libname}_delayed PROPERTIES LINKER_LANGUAGE "IMPLIB_DELAYED" PREFIX "")
endfunction()

# Cute little hack to produce import libs
set(CMAKE_IMPLIB_CREATE_STATIC_LIBRARY "${CMAKE_DLLTOOL} --def <OBJECTS> --kill-at --output-lib=<TARGET>")
set(CMAKE_IMPLIB_DELAYED_CREATE_STATIC_LIBRARY "${CMAKE_DLLTOOL} --def <OBJECTS> --kill-at --output-delaylib=<TARGET>")
function(spec2def _dllname _spec_file)
    # do we also want to add importlib targets?
    if(${ARGC} GREATER 2)
        if(${ARGN} STREQUAL "ADD_IMPORTLIB")
            set(__add_importlib TRUE)
        else()
            message(FATAL_ERROR "Wrong argument passed to spec2def, ${ARGN}")
        endif()
    endif()

    # get library basename
    get_filename_component(_file ${_dllname} NAME_WE)

    # error out on anything else than spec
    if(NOT ${_spec_file} MATCHES ".*\\.spec")
        message(FATAL_ERROR "spec2def only takes spec files as input.")
    endif()

    # generate exports def and stubs C file for the module
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_file}.def ${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c
        COMMAND native-spec2def -n=${_dllname} --kill-at -a=${ARCH2} -d=${CMAKE_CURRENT_BINARY_DIR}/${_file}.def -s=${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file} native-spec2def)

    if(__add_importlib)
        generate_import_lib(lib${_file} ${_dllname} ${_spec_file})
    endif()
endfunction()

macro(macro_mc FLAG FILE)
    set(COMMAND_MC ${CMAKE_MC_COMPILER} ${FLAG} -b ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.mc -r ${REACTOS_BINARY_DIR}/include/reactos -h ${REACTOS_BINARY_DIR}/include/reactos)
endmacro()

#pseh lib, needed with mingw
set(PSEH_LIB "pseh")

# Macros
if(PCH)
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
        if(_target_defs)
            foreach(item ${_target_defs})
                list(APPEND ${_out_compile_flags} -D${item})
            endforeach()
        endif()

        if(IS_CPP)
            list(APPEND ${_out_compile_flags} ${CMAKE_CXX_FLAGS})
        else()
            list(APPEND ${_out_compile_flags} ${CMAKE_C_FLAGS})
        endif()

        separate_arguments(${_out_compile_flags})
    endmacro()

    macro(add_pch _target_name _FILE)
        set(_header_filename ${CMAKE_CURRENT_SOURCE_DIR}/${_FILE})
        get_filename_component(_basename ${_FILE} NAME)
        set(_gch_filename ${_basename}.gch)
        _PCH_GET_COMPILE_FLAGS(${_target_name} _args ${_header_filename})

        if(IS_CPP)
            set(__lang CXX)
            set(__compiler ${CCACHE} ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ARG1})
        else()
            set(__lang C)
            set(__compiler ${CCACHE} ${CMAKE_C_COMPILER} ${CMAKE_C_COMPILER_ARG1})
        endif()

        add_custom_command(OUTPUT ${_gch_filename}
            COMMAND ${__compiler} ${_args}
            IMPLICIT_DEPENDS ${__lang} ${_header_filename}
            DEPENDS ${_header_filename} ${ARGN})
        get_target_property(_src_files ${_target_name} SOURCES)
        add_target_compile_flags(${_target_name} "-fpch-preprocess -Winvalid-pch -Wno-error=invalid-pch")
        foreach(_item in ${_src_files})
            get_source_file_property(__src_lang ${_item} LANGUAGE)
            if(__src_lang STREQUAL __lang)
                set_source_files_properties(${_item} PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_gch_filename})
            endif()
        endforeach()
        #set dependency checking : depends on precompiled header only which already depends on deeper header
        set_target_properties(${_target_name} PROPERTIES IMPLICIT_DEPENDS_INCLUDE_TRANSFORM "\"${_basename}\"=;<${_basename}>=")
    endmacro()
else()
    macro(add_pch _target_name _FILE)
    endmacro()
endif()

function(CreateBootSectorTarget _target_name _asm_file _binary_file _base_address)
    set(_object_file ${_binary_file}.o)

    add_custom_command(
        OUTPUT ${_object_file}
        COMMAND ${CMAKE_ASM_COMPILER} -x assembler-with-cpp -o ${_object_file} -I${REACTOS_SOURCE_DIR}/include/asm -I${REACTOS_BINARY_DIR}/include/asm -D__ASM__ -c ${_asm_file}
        DEPENDS ${_asm_file})

    add_custom_command(
        OUTPUT ${_binary_file}
        COMMAND native-obj2bin ${_object_file} ${_binary_file} ${_base_address}
        # COMMAND objcopy --output-target binary --image-base 0x${_base_address} ${_object_file} ${_binary_file}
        DEPENDS ${_object_file} native-obj2bin)

    set_source_files_properties(${_object_file} ${_binary_file} PROPERTIES GENERATED TRUE)

    add_custom_target(${_target_name} ALL DEPENDS ${_binary_file})

endfunction()

function(allow_warnings __module)
    add_target_compile_flags(${__module} "-Wno-error")
endfunction()
