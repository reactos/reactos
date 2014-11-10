
#if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # no optimization
elseif(OPTIMIZE STREQUAL "1")
    add_definitions(/O1)
elseif(OPTIMIZE STREQUAL "2")
    add_definitions(/O2)
elseif(OPTIMIZE STREQUAL "3")
    add_definitions(/Ot /Ox /GS-)
elseif(OPTIMIZE STREQUAL "4")
    add_definitions(/Os /Ox /GS-)
elseif(OPTIMIZE STREQUAL "5")
    add_definitions(/GF /Gy /Ob2 /Os /Ox /GS-)
endif()

if(ARCH STREQUAL "i386")
    add_definitions(/DWIN32 /D_WINDOWS)
endif()

add_definitions(/Dinline=__inline /D__STDC__=1)

add_compile_flags("/X /GR- /EHs-c- /GS- /Zl /W3")

# HACK: for VS 11+ we need to explicitly disable SSE, which is off by
# default for older compilers. See CORE-6507
if(MSVC_VERSION GREATER 1699 AND ARCH STREQUAL "i386")
    add_compile_flags("/arch:IA32")
endif ()

# VS 12+ requires /FS when used in parallel compilations
if(MSVC_VERSION GREATER 1799 AND NOT MSVC_IDE)
    add_compile_flags("/FS")
endif ()

# Disable overly sensitive warnings as well as those that generally aren't
# useful to us.
# - TODO: C4018: signed/unsigned mismatch
# - C4244: implicit integer truncation
# - C4290: C++ exception specification ignored
#add_compile_flags("/wd4018 /wd4244 /wd4290")
add_compile_flags("/wd4290 /wd4244")

# The following warnings are treated as errors:
# - C4013: implicit function declaration
# - C4020: too many actual parameters
# - C4022: pointer type mismatch for parameter
# - TODO: C4028: formal parameter different from declaration
# - C4047: different level of indirection
# - TODO: C4090: different 'modifier' qualifiers (for C programs only;
#          for C++ programs, the compiler error C2440 is issued)
# - C4098: void function returning a value
# - C4113: parameter lists differ
# - C4129: unrecognized escape sequence
# - TODO: C4133: incompatible types
# - C4163: 'identifier': not available as an intrinsic function
# - C4229: modifiers on data are ignored
# - C4700: uninitialized variable usage
# - C4603: macro is not defined or definition is different after precompiled header use
add_compile_flags("/we4013 /we4020 /we4022 /we4047 /we4098 /we4113 /we4129 /we4163 /we4229 /we4700 /we4603")

# Enable warnings above the default level, but don't treat them as errors:
# - C4115: named type definition in parentheses
add_compile_flags("/w14115")

# Debugging
#if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(NOT (_PREFAST_ OR _VS_ANALYZE_))
        add_compile_flags("/Zi")
    endif()
    add_compile_flags("/Ob0 /Od")
#elseif(${CMAKE_BUILD_TYPE} STREQUAL "Release")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_flags("/Ob2 /D NDEBUG")
endif()

if(MSVC_IDE)
    add_compile_flags("/MP")
    if(NOT DEFINED USE_FOLDER_STRUCTURE)
        set(USE_FOLDER_STRUCTURE FALSE)
    endif()
endif()

if(NOT DEFINED RUNTIME_CHECKS)
    set(RUNTIME_CHECKS FALSE)
endif()

if(RUNTIME_CHECKS)
    add_definitions(-D__RUNTIME_CHECKS__)
    add_compile_flags("/RTC1")
endif()

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO /INCREMENTAL:NO /SAFESEH:NO /NODEFAULTLIB /RELEASE")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO /INCREMENTAL:NO /SAFESEH:NO /NODEFAULTLIB /RELEASE")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /MANIFEST:NO /INCREMENTAL:NO /SAFESEH:NO /NODEFAULTLIB /RELEASE")

if(CMAKE_DISABLE_NINJA_DEPSLOG)
    set(cl_includes_flag "")
else()
    set(cl_includes_flag "/showIncludes")
endif()

if(MSVC_IDE AND (CMAKE_VERSION MATCHES "ReactOS"))
    # For VS builds we'll only have en-US in resource files
    add_definitions(/DLANGUAGE_EN_US)
else()
    # Only VS 10+ resource compiler supports /nologo
    if(MSVC_VERSION GREATER 1599)
        set(rc_nologo_flag "/nologo")
    else()
        set(rc_nologo_flag)
    endif()
    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> ${rc_nologo_flag} <FLAGS> <DEFINES> ${I18N_DEFS} /fo<OBJECT> <SOURCE>")
    if(ARCH STREQUAL "arm")
        set(CMAKE_ASM_COMPILE_OBJECT
            "cl ${cl_includes_flag} /nologo /X /I${REACTOS_SOURCE_DIR}/include/asm /I${REACTOS_BINARY_DIR}/include/asm <FLAGS> <DEFINES> /D__ASM__ /D_USE_ML /EP /c <SOURCE> > <OBJECT>.tmp"
            "<CMAKE_ASM_COMPILER> -nologo -o <OBJECT> <OBJECT>.tmp")
    else()
        set(CMAKE_ASM_COMPILE_OBJECT
            "cl ${cl_includes_flag} /nologo /X /I${REACTOS_SOURCE_DIR}/include/asm /I${REACTOS_BINARY_DIR}/include/asm <FLAGS> <DEFINES> /D__ASM__ /D_USE_ML /EP /c <SOURCE> > <OBJECT>.tmp"
            "<CMAKE_ASM_COMPILER> /nologo /Cp /Fo<OBJECT> /c /Ta <OBJECT>.tmp")
    endif()
endif()

if(_VS_ANALYZE_)
    message("VS static analysis enabled!")
    add_compile_flags("/analyze")
elseif(_PREFAST_)
    message("PREFAST enabled!")
    set(CMAKE_C_COMPILE_OBJECT "prefast <CMAKE_C_COMPILER> ${CMAKE_START_TEMP_FILE} ${CMAKE_CL_NOLOGO} <FLAGS> <DEFINES> /Fo<OBJECT> -c <SOURCE>${CMAKE_END_TEMP_FILE}"
    "prefast LIST")
    set(CMAKE_CXX_COMPILE_OBJECT "prefast <CMAKE_CXX_COMPILER> ${CMAKE_START_TEMP_FILE} ${CMAKE_CL_NOLOGO} <FLAGS> <DEFINES> /TP /Fo<OBJECT> -c <SOURCE>${CMAKE_END_TEMP_FILE}"
    "prefast LIST")
    set(CMAKE_C_LINK_EXECUTABLE
    "<CMAKE_C_COMPILER> ${CMAKE_CL_NOLOGO} <OBJECTS> ${CMAKE_START_TEMP_FILE} <FLAGS> /Fe<TARGET> -link /implib:<TARGET_IMPLIB> /version:<TARGET_VERSION_MAJOR>.<TARGET_VERSION_MINOR> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES>${CMAKE_END_TEMP_FILE}")
    set(CMAKE_CXX_LINK_EXECUTABLE
    "<CMAKE_CXX_COMPILER> ${CMAKE_CL_NOLOGO} <OBJECTS> ${CMAKE_START_TEMP_FILE} <FLAGS> /Fe<TARGET> -link /implib:<TARGET_IMPLIB> /version:<TARGET_VERSION_MAJOR>.<TARGET_VERSION_MINOR> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES>${CMAKE_END_TEMP_FILE}")
endif()

set(CMAKE_RC_CREATE_SHARED_LIBRARY ${CMAKE_C_CREATE_SHARED_LIBRARY})
set(CMAKE_ASM_CREATE_SHARED_LIBRARY ${CMAKE_C_CREATE_SHARED_LIBRARY})
set(CMAKE_ASM_CREATE_STATIC_LIBRARY ${CMAKE_C_CREATE_STATIC_LIBRARY})

if(PCH)
    macro(add_pch _target _pch _sources)

        # Workaround for the MSVC toolchain (MSBUILD) /MP bug
        set(_temp_gch ${CMAKE_CURRENT_BINARY_DIR}/${_target}.pch)
        if(MSVC_IDE)
            file(TO_NATIVE_PATH ${_temp_gch} _gch)
        else()
            set(_gch ${_temp_gch})
        endif()

        if(IS_CPP)
            set(_pch_language CXX)
            set(_cl_lang_flag "/TP")
        else()
            set(_pch_language C)
            set(_cl_lang_flag "/TC")
        endif()

        if(MSVC_IDE)
            set(_pch_path_name_flag "/Fp${_gch}")
        endif()

        # Build the precompiled header
        # HEADER_FILE_ONLY FALSE: force compiling the header
        set_source_files_properties(${_pch} PROPERTIES
            HEADER_FILE_ONLY FALSE
            LANGUAGE ${_pch_language}
            COMPILE_FLAGS "${_cl_lang_flag} /Yc /Fp${_gch}"
            OBJECT_OUTPUTS ${_gch})

        # Prevent a race condition related to writing to the PDB files between the PCH and the excluded list of source files
        get_target_property(_target_sources ${_target} SOURCES)
        list(REMOVE_ITEM _target_sources ${_pch})
        foreach(_target_src ${_target_sources})
            set_property(SOURCE ${_target_src} APPEND PROPERTY OBJECT_DEPENDS ${_gch})
        endforeach()

        # Use the precompiled header with the specified source files, skipping the pch itself
        list(REMOVE_ITEM ${_sources} ${_pch})
        foreach(_src ${${_sources}})
            set_property(SOURCE ${_src} APPEND_STRING PROPERTY COMPILE_FLAGS " /FI${_gch} /Yu${_gch} ${_pch_path_name_flag}")
        endforeach()
    endmacro()
else()
    macro(add_pch _target _pch _sources)
    endmacro()
endif()

function(set_entrypoint _module _entrypoint)
    if(${_entrypoint} STREQUAL "0")
        add_target_link_flags(${_module} "/NOENTRY")
    elseif(ARCH STREQUAL "i386")
        set(_entrysymbol ${_entrypoint})
        if(${ARGC} GREATER 2)
            set(_entrysymbol ${_entrysymbol}@${ARGV2})
        endif()
        add_target_link_flags(${_module} "/ENTRY:${_entrysymbol}")
    else()
        add_target_link_flags(${_module} "/ENTRY:${_entrypoint}")
    endif()
endfunction()

function(set_subsystem MODULE SUBSYSTEM)
    if(ARCH STREQUAL "amd64")
        add_target_link_flags(${MODULE} "/SUBSYSTEM:${SUBSYSTEM},5.02")
    else()
        add_target_link_flags(${MODULE} "/SUBSYSTEM:${SUBSYSTEM},5.01")
    endif()
endfunction()

function(set_image_base MODULE IMAGE_BASE)
    add_target_link_flags(${MODULE} "/BASE:${IMAGE_BASE}")
endfunction()

function(set_module_type_toolchain MODULE TYPE)
    if(CPP_USE_STL)
        if((${TYPE} STREQUAL "kernelmodedriver") OR (${TYPE} STREQUAL "wdmdriver"))
            message(FATAL_ERROR "Use of STL in kernelmodedriver or wdmdriver type module prohibited")
        endif()
        target_link_libraries(${MODULE} cpprt stlport oldnames)
    elseif(CPP_USE_RT)
        target_link_libraries(${MODULE} cpprt)
    endif()
    if((${TYPE} STREQUAL "win32dll") OR (${TYPE} STREQUAL "win32ocx") OR (${TYPE} STREQUAL "cpl"))
        add_target_link_flags(${MODULE} "/DLL")
    elseif(${TYPE} STREQUAL "kernelmodedriver")
        add_target_link_flags(${MODULE} "/DRIVER")
    elseif(${TYPE} STREQUAL "wdmdriver")
        add_target_link_flags(${MODULE} "/DRIVER:WDM")
    endif()

    if(RUNTIME_CHECKS)
        target_link_libraries(${MODULE} runtmchk)
    endif()

endfunction()

# Define those for having real libraries
set(CMAKE_IMPLIB_CREATE_STATIC_LIBRARY "LINK /LIB /NOLOGO <LINK_FLAGS> /OUT:<TARGET> <OBJECTS>")

if(ARCH STREQUAL "arm")
    set(CMAKE_STUB_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> -nologo -o <OBJECT> <SOURCE>")
else()
    set(CMAKE_STUB_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> /nologo /Cp /Fo<OBJECT> /c /Ta <SOURCE>")
endif()

function(add_delay_importlibs _module)
    get_target_property(_module_type ${_module} TYPE)
    if(_module_type STREQUAL "STATIC_LIBRARY")
        message(FATAL_ERROR "Cannot add delay imports to a static library")
    endif()
    foreach(_lib ${ARGN})
        add_target_link_flags(${_module} "/DELAYLOAD:${_lib}.dll")
        target_link_libraries(${_module} lib${_lib})
    endforeach()
    target_link_libraries(${_module} delayimp)
endfunction()

function(generate_import_lib _libname _dllname _spec_file)

    set(_def_file ${CMAKE_CURRENT_BINARY_DIR}/${_libname}_exp.def)
    set(_asm_stubs_file ${CMAKE_CURRENT_BINARY_DIR}/${_libname}_stubs.asm)

    # Generate the asm stub file and the def file for import library
    add_custom_command(
        OUTPUT ${_asm_stubs_file} ${_def_file}
        COMMAND native-spec2def --ms -a=${SPEC2DEF_ARCH} --implib -n=${_dllname} -d=${_def_file} -l=${_asm_stubs_file} ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file} native-spec2def)

    if(MSVC_IDE)
        # Compile the generated asm stub file
        if(ARCH STREQUAL "arm")
            set(_asm_stub_command ${CMAKE_ASM_COMPILER} -nologo -o ${_asm_stubs_file}.obj ${_asm_stubs_file})
        else()
            set(_asm_stub_command ${CMAKE_ASM_COMPILER} /Cp /Fo${_asm_stubs_file}.obj /c /Ta ${_asm_stubs_file})
        endif()
        add_custom_command(
            OUTPUT ${_asm_stubs_file}.obj
            COMMAND ${_asm_stub_command}
            DEPENDS ${_asm_stubs_file})
    else()
        # Be clear about the "language"
        # Thanks MS for creating a stupid linker
        set_source_files_properties(${_asm_stubs_file} PROPERTIES LANGUAGE "STUB_ASM")
    endif()

    # Add our library
    if(MSVC_IDE)
        add_library(${_libname} STATIC EXCLUDE_FROM_ALL ${_asm_stubs_file}.obj)
        set_source_files_properties(${_asm_stubs_file}.obj PROPERTIES EXTERNAL_OBJECT 1)
        set_target_properties(${_libname} PROPERTIES LINKER_LANGUAGE "C")
    else()
        # NOTE: as stub file and def file are generated in one pass, depending on one is like depending on the other
        add_library(${_libname} STATIC EXCLUDE_FROM_ALL ${_asm_stubs_file})
        # set correct "link rule"
        set_target_properties(${_libname} PROPERTIES LINKER_LANGUAGE "IMPLIB")
    endif()
    set_target_properties(${_libname} PROPERTIES STATIC_LIBRARY_FLAGS "/DEF:${_def_file}")
endfunction()

if(ARCH STREQUAL "amd64")
    # This is NOT a typo.
    # See https://software.intel.com/en-us/forums/topic/404643
    add_definitions(/D__x86_64)
    set(SPEC2DEF_ARCH x86_64)
elseif(ARCH STREQUAL "arm")
    add_definitions(/D__arm__)
    set(SPEC2DEF_ARCH arm)
else()
    set(SPEC2DEF_ARCH i386)
endif()
function(spec2def _dllname _spec_file)
    # Do we also want to add importlib targets?
    if(${ARGC} GREATER 2)
        if(${ARGN} STREQUAL "ADD_IMPORTLIB")
            set(__add_importlib TRUE)
        else()
            message(FATAL_ERROR "Wrong argument passed to spec2def, ${ARGN}")
        endif()
    endif()

    # Get library basename
    get_filename_component(_file ${_dllname} NAME_WE)

    # Error out on anything else than spec
    if(NOT ${_spec_file} MATCHES ".*\\.spec")
        message(FATAL_ERROR "spec2def only takes spec files as input.")
    endif()

    # Generate exports def and C stubs file for the DLL
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_file}.def ${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c
        COMMAND native-spec2def --ms -a=${SPEC2DEF_ARCH} -n=${_dllname} -d=${CMAKE_CURRENT_BINARY_DIR}/${_file}.def -s=${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file} native-spec2def)

    if(__add_importlib)
        generate_import_lib(lib${_file} ${_dllname} ${_spec_file})
    endif()
endfunction()

macro(macro_mc FLAG FILE)
    set(COMMAND_MC ${CMAKE_MC_COMPILER} ${FLAG} -b ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.mc -r ${REACTOS_BINARY_DIR}/include/reactos -h ${REACTOS_BINARY_DIR}/include/reactos)
endmacro()

# PSEH workaround
set(PSEH_LIB "pseh")

# Use a full path for the x86 version of ml when using x64 VS.
# It's not a problem when using the DDK/WDK because, in x64 mode,
# both the x86 and x64 versions of ml are available.
if((ARCH STREQUAL "amd64") AND (DEFINED ENV{VCINSTALLDIR}))
    set(CMAKE_ASM16_COMPILER $ENV{VCINSTALLDIR}/bin/ml.exe)
elseif(ARCH STREQUAL "arm")
    set(CMAKE_ASM16_COMPILER armasm.exe)
else()
    set(CMAKE_ASM16_COMPILER ml.exe)
endif()

function(CreateBootSectorTarget _target_name _asm_file _binary_file _base_address)
    set(_object_file ${_binary_file}.obj)
    set(_temp_file ${_binary_file}.tmp)

    add_custom_command(
        OUTPUT ${_temp_file}
        COMMAND ${CMAKE_C_COMPILER} /nologo /X /I${REACTOS_SOURCE_DIR}/include/asm /I${REACTOS_BINARY_DIR}/include/asm /I${REACTOS_SOURCE_DIR}/boot/freeldr /D__ASM__ /D_USE_ML /EP /c ${_asm_file} > ${_temp_file}
        DEPENDS ${_asm_file})

    if(ARCH STREQUAL "arm")
        set(_asm16_command ${CMAKE_ASM16_COMPILER} -nologo -o ${_object_file} ${_temp_file})
    else()
        set(_asm16_command ${CMAKE_ASM16_COMPILER} /nologo /Cp /Fo${_object_file} /c /Ta ${_temp_file})
    endif()

    add_custom_command(
        OUTPUT ${_object_file}
        COMMAND ${_asm16_command}
        DEPENDS ${_temp_file})

    add_custom_command(
        OUTPUT ${_binary_file}
        COMMAND native-obj2bin ${_object_file} ${_binary_file} ${_base_address}
        DEPENDS ${_object_file} native-obj2bin)

    set_source_files_properties(${_object_file} ${_temp_file} ${_binary_file} PROPERTIES GENERATED TRUE)

    add_custom_target(${_target_name} ALL DEPENDS ${_binary_file})
endfunction()

function(allow_warnings __module)
endfunction()

macro(add_asm_files _target)
    if(MSVC_IDE AND (CMAKE_VERSION MATCHES "ReactOS"))
        get_defines(_directory_defines)
        get_includes(_directory_includes)
        get_directory_property(_defines COMPILE_DEFINITIONS)
        foreach(_source_file ${ARGN})
            get_filename_component(_source_file_base_name ${_source_file} NAME_WE)
            get_filename_component(_source_file_full_path ${_source_file} ABSOLUTE)
            set(_preprocessed_asm_file ${CMAKE_CURRENT_BINARY_DIR}/asm/${_source_file_base_name}_${_target}.tmp)
            set(_object_file ${CMAKE_CURRENT_BINARY_DIR}/asm/${_source_file_base_name}_${_target}.obj)
            get_source_file_property(_defines_semicolon_list ${_source_file_full_path} COMPILE_DEFINITIONS)
            unset(_source_file_defines)
            foreach(_define ${_defines_semicolon_list})
                if(NOT ${_define} STREQUAL "NOTFOUND")
                    list(APPEND _source_file_defines -D${_define})
                endif()
            endforeach()
            if(ARCH STREQUAL "arm")
                set(_pp_asm_compile_command ${CMAKE_ASM_COMPILER} -nologo -o ${_object_file} ${_preprocessed_asm_file})
            else()
                set(_pp_asm_compile_command ${CMAKE_ASM_COMPILER} /nologo /Cp /Fo${_object_file} /c /Ta ${_preprocessed_asm_file})
            endif()
            add_custom_command(
                OUTPUT ${_preprocessed_asm_file} ${_object_file}
                COMMAND cl /nologo /X /I${REACTOS_SOURCE_DIR}/include/asm /I${REACTOS_BINARY_DIR}/include/asm ${_directory_includes} ${_source_file_defines} ${_directory_defines} /D__ASM__ /D_USE_ML /EP /c ${_source_file_full_path} > ${_preprocessed_asm_file} && ${_pp_asm_compile_command}
                DEPENDS ${_source_file_full_path})
            set_source_files_properties(${_object_file} PROPERTIES EXTERNAL_OBJECT 1)
            list(APPEND ${_target} ${_object_file})
        endforeach()
    else()
        list(APPEND ${_target} ${ARGN})
    endif()
endmacro()
