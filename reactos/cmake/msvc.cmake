
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

add_compile_flags("/X /GR- /GS- /Zl /W3")

# HACK: for VS 11+ we need to explicitly disable SSE, which is off by
# default for older compilers. See bug #7174
if (MSVC_VERSION GREATER 1699 AND ARCH STREQUAL "i386")
    add_compile_flags("/arch:IA32")
endif ()

# C4700 is almost always likely to result in broken code, so mark it as an error
add_compile_flags("/we4700")

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
endif()

if(${_MACHINE_ARCH_FLAG} MATCHES X86)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO /INCREMENTAL:NO /SAFESEH:NO /NODEFAULTLIB /RELEASE")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO /INCREMENTAL:NO /SAFESEH:NO /NODEFAULTLIB /RELEASE")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /MANIFEST:NO /INCREMENTAL:NO /SAFESEH:NO /NODEFAULTLIB /RELEASE")
endif()

if(MSVC_IDE AND (CMAKE_VERSION MATCHES "ReactOS"))
    # for VS builds we'll only have en-US in resource files
    add_definitions(/DLANGUAGE_EN_US)
else()
    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> /nologo <FLAGS> <DEFINES> ${I18N_DEFS} /fo<OBJECT> <SOURCE>")
    set(CMAKE_ASM_COMPILE_OBJECT
        "cl /nologo /X /I${REACTOS_SOURCE_DIR}/include/asm /I${REACTOS_BINARY_DIR}/include/asm <FLAGS> <DEFINES> /D__ASM__ /D_USE_ML /EP /c <SOURCE> > <OBJECT>.tmp"
        "<CMAKE_ASM_COMPILER> /nologo /Cp /Fo<OBJECT> /c /Ta <OBJECT>.tmp")
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

macro(add_pch _target_name _FILE)
endmacro()

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
    add_target_link_flags(${MODULE} "/SUBSYSTEM:${SUBSYSTEM}")
endfunction()

function(set_image_base MODULE IMAGE_BASE)
    add_target_link_flags(${MODULE} "/BASE:${IMAGE_BASE}")
endfunction()

function(set_module_type_toolchain MODULE TYPE)
    if((${TYPE} STREQUAL "win32dll") OR (${TYPE} STREQUAL "win32ocx") OR (${TYPE} STREQUAL "cpl"))
        add_target_link_flags(${MODULE} "/DLL")
    elseif(${TYPE} STREQUAL "kernelmodedriver")
        add_target_link_flags(${MODULE} "/DRIVER")
    elseif(${TYPE} STREQUAL "wdmdriver")
        add_target_link_flags(${MODULE} "/DRIVER:WDM")
    endif()
endfunction()

#define those for having real libraries
set(CMAKE_IMPLIB_CREATE_STATIC_LIBRARY "LINK /LIB /NOLOGO <LINK_FLAGS> /OUT:<TARGET> <OBJECTS>")
set(CMAKE_STUB_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> /Cp /Fo<OBJECT> /c /Ta <SOURCE>")
macro(add_delay_importlibs MODULE)
    foreach(LIB ${ARGN})
        add_target_link_flags(${MODULE} "/DELAYLOAD:${LIB}.dll")
        target_link_libraries(${MODULE} lib${LIB})
    endforeach()
    target_link_libraries(${MODULE} delayimp)
endmacro()

function(generate_import_lib _libname _dllname _spec_file)

    set(_def_file ${CMAKE_CURRENT_BINARY_DIR}/${_libname}_exp.def)
    set(_asm_stubs_file ${CMAKE_CURRENT_BINARY_DIR}/${_libname}_stubs.asm)
    
    # Generate the asm stub file and the def file for import library
    add_custom_command(
        OUTPUT ${_asm_stubs_file} ${_def_file}
        COMMAND native-spec2def --ms --kill-at -a=${SPEC2DEF_ARCH} --implib -n=${_dllname} -d=${_def_file} -l=${_asm_stubs_file} ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file} native-spec2def)
    
    if(MSVC_IDE)
        # Compile the generated asm stub file
        add_custom_command(
            OUTPUT ${_asm_stubs_file}.obj
            COMMAND ${CMAKE_ASM_COMPILER} /Cp /Fo${_asm_stubs_file}.obj /c /Ta ${_asm_stubs_file}
            DEPENDS ${_asm_stubs_file})
    else()
        # be clear about the "language"
        # Thanks MS for creating a stupid linker
        set_source_files_properties(${_asm_stubs_file} PROPERTIES LANGUAGE "STUB_ASM")
    endif()

    # add our library
    if(MSVC_IDE)
        add_library(${_libname} STATIC EXCLUDE_FROM_ALL ${_asm_stubs_file}.obj)
        set_source_files_properties(${_asm_stubs_file}.obj PROPERTIES EXTERNAL_OBJECT 1)
        set_target_properties(${_libname} PROPERTIES LINKER_LANGUAGE "C")
    else()
        # NOTE: as stub file and def file are generated in one pass, depending on one is like depending on the other
        add_library(${_libname} STATIC EXCLUDE_FROM_ALL ${_asm_stubs_file})
        add_dependencies(${_libname} ${_def_file})
        # set correct "link rule"
        set_target_properties(${_libname} PROPERTIES LINKER_LANGUAGE "IMPLIB")
    endif()
    set_target_properties(${_libname} PROPERTIES STATIC_LIBRARY_FLAGS "/DEF:${_def_file}")
endfunction()

if(ARCH STREQUAL "amd64")
    add_definitions(/D__x86_64)
    set(SPEC2DEF_ARCH x86_64)
else()
    set(SPEC2DEF_ARCH i386)
endif()
function(spec2def _dllname _spec_file)
    # do we also want to add impotlib targets?
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

    #generate def for the DLL and C stubs file
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_file}.def ${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c
        COMMAND native-spec2def --ms --kill-at -a=${SPEC2DEF_ARCH} -n=${_dllname} -d=${CMAKE_CURRENT_BINARY_DIR}/${_file}.def -s=${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file} native-spec2def)

    if(__add_importlib)
        generate_import_lib(lib${_file} ${_dllname} ${_spec_file})
    endif()
endfunction()

macro(macro_mc FLAG FILE)
    set(COMMAND_MC ${CMAKE_MC_COMPILER} ${FLAG} -r ${REACTOS_BINARY_DIR}/include/reactos -h ${REACTOS_BINARY_DIR}/include/reactos ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.mc)
endmacro()

#pseh workaround
set(PSEH_LIB "pseh")

# Use a full path for the x86 version of ml when using x64 VS.
# It's not a problem when using the DDK/WDK because, in x64 mode,
# both the x86 and x64 versions of ml are available.
if((ARCH STREQUAL "amd64") AND (DEFINED ENV{VCINSTALLDIR}))
    set(CMAKE_ASM16_COMPILER $ENV{VCINSTALLDIR}/bin/ml.exe)
else()
    set(CMAKE_ASM16_COMPILER ml.exe)
endif()

function(CreateBootSectorTarget _target_name _asm_file _binary_file _base_address)
    set(_object_file ${_binary_file}.obj)
    set(_temp_file ${_binary_file}.tmp)

    add_custom_command(
        OUTPUT ${_temp_file}
        COMMAND ${CMAKE_C_COMPILER} /nologo /X /I${REACTOS_SOURCE_DIR}/include/asm /I${REACTOS_BINARY_DIR}/include/asm /D__ASM__ /D_USE_ML /EP /c ${_asm_file} > ${_temp_file}
        DEPENDS ${_asm_file})

    add_custom_command(
        OUTPUT ${_object_file}
        COMMAND ${CMAKE_ASM16_COMPILER} /nologo /Cp /Fo${_object_file} /c /Ta ${_temp_file}
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
            set(_preprocessed_asm_file ${CMAKE_CURRENT_BINARY_DIR}/asm/${_source_file_base_name}_${_target}.tmp)
            set(_object_file ${CMAKE_CURRENT_BINARY_DIR}/asm/${_source_file_base_name}_${_target}.obj)
            set(_source_file_full_path ${CMAKE_CURRENT_SOURCE_DIR}/${_source_file})
            get_source_file_property(_defines_semicolon_list ${_source_file_full_path} COMPILE_DEFINITIONS)
            unset(_source_file_defines)
            foreach(_define ${_defines_semicolon_list})
                if(NOT ${_define} STREQUAL "NOTFOUND")
                    list(APPEND _source_file_defines -D${_define})
                endif()
            endforeach()
            add_custom_command(
                OUTPUT ${_preprocessed_asm_file} ${_object_file}
                COMMAND cl /nologo /X /I${REACTOS_SOURCE_DIR}/include/asm /I${REACTOS_BINARY_DIR}/include/asm ${_directory_includes} ${_source_file_defines} ${_directory_defines} /D__ASM__ /D_USE_ML /EP /c ${_source_file_full_path} > ${_preprocessed_asm_file} && ${CMAKE_ASM_COMPILER} /nologo /Cp /Fo${_object_file} /c /Ta ${_preprocessed_asm_file}
                DEPENDS ${_source_file_full_path})
            set_source_files_properties(${_object_file} PROPERTIES EXTERNAL_OBJECT 1)
            list(APPEND ${_target} ${_object_file})
        endforeach()
    else()
        list(APPEND ${_target} ${ARGN})
    endif()
endmacro()
