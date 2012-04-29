
if(${CMAKE_BUILD_TYPE} MATCHES Debug)
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

if(ARCH MATCHES i386)
    add_definitions(/DWIN32 /D_WINDOWS)
endif()

add_definitions(/Dinline=__inline /D__STDC__=1)

add_compile_flags("/X /GR- /GS- /Zl /W3")

# C4700 is almost always likely to result in broken code, so mark it as an error
add_compile_flags("/we4700")

# Debugging
if(${CMAKE_BUILD_TYPE} MATCHES Debug)
    if(NOT _PREFAST_)
        add_compile_flags("/Zi")
    endif()
    add_compile_flags("/Ob0 /Od")
elseif(${CMAKE_BUILD_TYPE} MATCHES Release)
    add_compile_flags("/Ob2 /D NDEBUG")
endif()

if(${_MACHINE_ARCH_FLAG} MATCHES X86)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO /NODEFAULTLIB /RELEASE")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO /NODEFAULTLIB /RELEASE")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO /NODEFAULTLIB /RELEASE")
endif()

set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> <DEFINES> ${I18N_DEFS} /fo<OBJECT> <SOURCE>")

if(MSVC_IDE)
    # Asm source files are not supported in VS generators yet. As a result, <DEFINES> isn't recognized.
    # We may temporarily use just the global defines, but this is not a solution as some modules (minihal for example) apply additional definitions to source files, so we get an incorrect build of such targets.
    get_directory_property(definitions DEFINITIONS)
    set(CMAKE_ASM_COMPILE_OBJECT
        "cl /nologo /X /I${REACTOS_SOURCE_DIR}/include/asm /I${REACTOS_BINARY_DIR}/include/asm <FLAGS> ${definitions} /D__ASM__ /D_USE_ML /EP /c <SOURCE> > <OBJECT>.tmp"
        "<CMAKE_ASM_COMPILER> /nologo /Cp /Fo<OBJECT> /c /Ta <OBJECT>.tmp")
else()
    # NMake Makefiles
    set(CMAKE_ASM_COMPILE_OBJECT
        "cl /nologo /X /I${REACTOS_SOURCE_DIR}/include/asm /I${REACTOS_BINARY_DIR}/include/asm <FLAGS> <DEFINES> /D__ASM__ /D_USE_ML /EP /c <SOURCE> > <OBJECT>.tmp"
        "<CMAKE_ASM_COMPILER> /nologo /Cp /Fo<OBJECT> /c /Ta <OBJECT>.tmp")
endif()

if(_PREFAST_)
    if(MSVC_VERSION EQUAL 1600 OR MSVC_VERSION GREATER 1600)
        add_compile_flags("/analyze")
    else()
        message("PREFAST enabled!")
        set(CMAKE_C_COMPILE_OBJECT "prefast cl ${CMAKE_START_TEMP_FILE} ${CMAKE_CL_NOLOGO} <FLAGS> <DEFINES> /Fo<OBJECT> -c <SOURCE>${CMAKE_END_TEMP_FILE}"
    "prefast LIST")
        set(CMAKE_CXX_COMPILE_OBJECT "prefast cl ${CMAKE_START_TEMP_FILE} ${CMAKE_CL_NOLOGO} <FLAGS> <DEFINES> /TP /Fo<OBJECT> -c <SOURCE>${CMAKE_END_TEMP_FILE}"
    "prefast LIST")
        set(CMAKE_C_LINK_EXECUTABLE
    "cl ${CMAKE_CL_NOLOGO} <OBJECTS> ${CMAKE_START_TEMP_FILE} <FLAGS> /Fe<TARGET> -link /implib:<TARGET_IMPLIB> /version:<TARGET_VERSION_MAJOR>.<TARGET_VERSION_MINOR> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES>${CMAKE_END_TEMP_FILE}")
        set(CMAKE_CXX_LINK_EXECUTABLE
    "cl ${CMAKE_CL_NOLOGO} <OBJECTS> ${CMAKE_START_TEMP_FILE} <FLAGS> /Fe<TARGET> -link /implib:<TARGET_IMPLIB> /version:<TARGET_VERSION_MAJOR>.<TARGET_VERSION_MINOR> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES>${CMAKE_END_TEMP_FILE}")
    endif()

endif()

set(CMAKE_RC_CREATE_SHARED_LIBRARY ${CMAKE_C_CREATE_SHARED_LIBRARY})
set(CMAKE_ASM_CREATE_SHARED_LIBRARY ${CMAKE_C_CREATE_SHARED_LIBRARY})
set(CMAKE_ASM_CREATE_STATIC_LIBRARY ${CMAKE_C_CREATE_STATIC_LIBRARY})

macro(add_pch _target_name _FILE)
endmacro()

function(set_entrypoint _module _entrypoint)
    if(${_entrypoint} STREQUAL "0")
        add_target_link_flags(${_module} "/NOENTRY")
    elseif(ARCH MATCHES i386)
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
    add_target_link_flags(${MODULE} "/subsystem:${SUBSYSTEM}")
endfunction()

function(set_image_base MODULE IMAGE_BASE)
    add_target_link_flags(${MODULE} "/BASE:${IMAGE_BASE}")
endfunction()

function(set_module_type_toolchain MODULE TYPE)
    if((${TYPE} STREQUAL win32dll) OR (${TYPE} STREQUAL win32ocx) OR (${TYPE} STREQUAL cpl))
        add_target_link_flags(${MODULE} "/DLL")
    elseif(${TYPE} STREQUAL kernelmodedriver)
        add_target_link_flags(${MODULE} "/DRIVER")
    endif()
endfunction()

function(set_rc_compiler)
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
    # Generate the asm stub file and the def file for import library
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_libname}_stubs.asm ${CMAKE_CURRENT_BINARY_DIR}/${_libname}_exp.def
        COMMAND native-spec2def --ms --kill-at -a=${SPEC2DEF_ARCH} --implib -n=${_dllname} -d=${CMAKE_CURRENT_BINARY_DIR}/${_libname}_exp.def -l=${CMAKE_CURRENT_BINARY_DIR}/${_libname}_stubs.asm ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file} native-spec2def)

    # be clear about the "language"
    # Thanks MS for creating a stupid linker
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${_libname}_stubs.asm PROPERTIES LANGUAGE "STUB_ASM")

    # add our library
    # NOTE: as stub file and def file are generated in one pass, depending on one is like depending on the other
    add_library(${_libname} STATIC EXCLUDE_FROM_ALL
        ${CMAKE_CURRENT_BINARY_DIR}/${_libname}_stubs.asm)

    add_dependencies(${_libname} ${CMAKE_CURRENT_BINARY_DIR}\\${_libname}_exp.def)

    # set correct "link rule"
    set_target_properties(${_libname} PROPERTIES LINKER_LANGUAGE "IMPLIB"
        STATIC_LIBRARY_FLAGS "/DEF:${CMAKE_CURRENT_BINARY_DIR}\\${_libname}_exp.def")
endfunction()

if(${ARCH} MATCHES amd64)
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
    set(COMMAND_MC mc ${FLAG} -r ${REACTOS_BINARY_DIR}/include/reactos -h ${REACTOS_BINARY_DIR}/include/reactos ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.mc)
endmacro()

#pseh workaround
set(PSEH_LIB "pseh")

# Use full path for ml when using x64 VS
if((ARCH MATCHES amd64) AND ($ENV{VCINSTALLDIR}))
    set(CMAKE_ASM16_COMPILER $ENV{VCINSTALLDIR}/bin/ml.exe)
else()
    set(CMAKE_ASM16_COMPILER ml.exe)
endif()

function(CreateBootSectorTarget _target_name _asm_file _binary_file _base_address)

    set(_object_file ${_binary_file}.obj)
    set(_temp_file ${_binary_file}.tmp)

    add_custom_command(
        OUTPUT ${_temp_file}
        COMMAND cl /nologo /X /I${REACTOS_SOURCE_DIR}/include/asm /I${REACTOS_BINARY_DIR}/include/asm /D__ASM__ /D_USE_ML /EP /c ${_asm_file} > ${_temp_file}
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
