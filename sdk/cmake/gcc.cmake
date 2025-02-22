
# Show a note about ccache build
if(ENABLE_CCACHE)
    message("-- Enabling ccache build - done")
    set(CMAKE_C_USE_RESPONSE_FILE_FOR_INCLUDES OFF)
    set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES OFF)
endif()

# PDB style debug info
if(NOT DEFINED SEPARATE_DBG)
    set(SEPARATE_DBG FALSE)
endif()

# Dwarf based builds (no rsym)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(NO_ROSSYM TRUE)
elseif(NOT DEFINED NO_ROSSYM)
    set(NO_ROSSYM FALSE)
endif()

if(NOT DEFINED USE_PSEH3)
    set(USE_PSEH3 1)
endif()

if(USE_PSEH3)
    add_definitions(-D_USE_PSEH3=1)
endif()

if(NOT DEFINED USE_DUMMY_PSEH)
    set(USE_DUMMY_PSEH 0)
endif()

if(USE_DUMMY_PSEH)
    add_definitions(-D_USE_DUMMY_PSEH=1)
endif()

if(STACK_PROTECTOR)
    add_compile_options(-fstack-protector-strong)
endif()

# Compiler Core
# note: -fno-common is default since GCC 10
add_compile_options(-pipe -fms-extensions -fno-strict-aliasing -fno-common)

# A long double is 64 bits
add_compile_options(-mlong-double-64)

# Prevent GCC from searching any of the default directories.
# The case for C++ is handled through the reactos_c++ INTERFACE library
add_compile_options("$<$<NOT:$<COMPILE_LANGUAGE:CXX>>:-nostdinc>")

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options("-Wno-unknown-pragmas")
    add_compile_options(-fno-aggressive-loop-optimizations)
    if (DBG)
        add_compile_options("$<$<COMPILE_LANGUAGE:C>:-Wold-style-declaration>")
    endif()

    # Disable all math intrinsics. The reason is that these are implicitly declared
    # extern by GCC, which causes inline functions to generate global symbols.
    # And since GCC is retarded, these symbols are not marked as weak, so they
    # conflict with each other in multiple compilation units.
    add_compile_options(-fno-builtin-acosf)
    add_compile_options(-fno-builtin-acosl)
    add_compile_options(-fno-builtin-asinf)
    add_compile_options(-fno-builtin-asinl)
    add_compile_options(-fno-builtin-atan2f)
    add_compile_options(-fno-builtin-atan2l)
    add_compile_options(-fno-builtin-atanf)
    add_compile_options(-fno-builtin-atanl)
    add_compile_options(-fno-builtin-ceilf)
    add_compile_options(-fno-builtin-ceill)
    add_compile_options(-fno-builtin-coshf)
    add_compile_options(-fno-builtin-coshl)
    add_compile_options(-fno-builtin-cosf)
    add_compile_options(-fno-builtin-cosl)
    add_compile_options(-fno-builtin-expf)
    add_compile_options(-fno-builtin-expl)
    add_compile_options(-fno-builtin-fabsf)
    add_compile_options(-fno-builtin-fabsl)
    add_compile_options(-fno-builtin-floorf)
    add_compile_options(-fno-builtin-floorl)
    add_compile_options(-fno-builtin-fmodf)
    add_compile_options(-fno-builtin-fmodl)
    add_compile_options(-fno-builtin-frexpf)
    add_compile_options(-fno-builtin-frexpl)
    add_compile_options(-fno-builtin-hypotf)
    add_compile_options(-fno-builtin-hypotl)
    add_compile_options(-fno-builtin-ldexpf)
    add_compile_options(-fno-builtin-ldexpl)
    add_compile_options(-fno-builtin-logf)
    add_compile_options(-fno-builtin-logl)
    add_compile_options(-fno-builtin-log10f)
    add_compile_options(-fno-builtin-log10l)
    add_compile_options(-fno-builtin-modff)
    add_compile_options(-fno-builtin-modfl)
    add_compile_options(-fno-builtin-powf)
    add_compile_options(-fno-builtin-powl)
    add_compile_options(-fno-builtin-sinhf)
    add_compile_options(-fno-builtin-sinhl)
    add_compile_options(-fno-builtin-sinf)
    add_compile_options(-fno-builtin-sinl)
    add_compile_options(-fno-builtin-sqrtf)
    add_compile_options(-fno-builtin-sqrtl)
    add_compile_options(-fno-builtin-tanhf)
    add_compile_options(-fno-builtin-tanhl)
    add_compile_options(-fno-builtin-tanf)
    add_compile_options(-fno-builtin-tanl)
    add_compile_options(-fno-builtin-feraiseexcept)
    add_compile_options(-fno-builtin-feupdateenv)

    if(CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 11)
        add_compile_options(-fno-builtin-ceil)
        add_compile_options(-fno-builtin-ceilf)
        add_compile_options(-fno-builtin-cos)
        add_compile_options(-fno-builtin-floor)
        add_compile_options(-fno-builtin-floorf)
        add_compile_options(-fno-builtin-pow)
        add_compile_options(-fno-builtin-sin)
        add_compile_options(-fno-builtin-sincos)
        add_compile_options(-fno-builtin-sqrt)
        add_compile_options(-fno-builtin-sqrtf)
    endif()
    if(CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 13)
        add_compile_options(-fno-builtin-erf)
        add_compile_options(-fno-builtin-erff)
    endif()

elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options("$<$<COMPILE_LANGUAGE:C>:-Wno-microsoft>")
    add_compile_options(-Wno-pragma-pack)
    add_compile_options(-fno-associative-math)

    if(CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 12.0)
        # disable "libcall optimization"
        # see https://mudongliang.github.io/2020/12/02/undefined-reference-to-stpcpy.html
        add_compile_options(-fno-builtin-stpcpy)
    endif()

    set(CMAKE_LINK_DEF_FILE_FLAG "")
    set(CMAKE_STATIC_LIBRARY_SUFFIX ".a")
    set(CMAKE_LINK_LIBRARY_SUFFIX "")
    set(CMAKE_CREATE_WIN32_EXE "")
    set(CMAKE_C_COMPILE_OPTIONS_PIC "")
    set(CMAKE_CXX_COMPILE_OPTIONS_PIC "")
    set(CMAKE_C_COMPILE_OPTIONS_PIE "")
    set(CMAKE_CXX_COMPILE_OPTIONS_PIE "")
    set(CMAKE_ASM_FLAGS_DEBUG "")
    set(CMAKE_C_FLAGS_DEBUG "")
    set(CMAKE_CXX_FLAGS_DEBUG "")
endif()

# Debugging
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    if(SEPARATE_DBG)
        add_compile_options(-gdwarf-2 -ggdb)
    else()
        add_compile_options(-gdwarf-2 -gstrict-dwarf)
        if(NOT CMAKE_C_COMPILER_ID STREQUAL Clang)
            add_compile_options(-femit-struct-debug-detailed=none -feliminate-unused-debug-symbols)
        endif()
    endif()
endif()

# Tuning
add_compile_options(-march=${OARCH} -mtune=${TUNE})

# Warnings, errors
if((NOT CMAKE_BUILD_TYPE STREQUAL "Release") AND (NOT CMAKE_C_COMPILER_ID STREQUAL Clang))
    add_compile_options(-Werror)
endif()

add_compile_options(-Wall -Wpointer-arith)

# Disable some overzealous warnings
add_compile_options(
    -Wno-unknown-warning-option
    -Wno-char-subscripts
    -Wno-multichar
    -Wno-unused-value
    -Wno-unused-const-variable
    -Wno-unused-local-typedefs
    -Wno-deprecated
    -Wno-unused-result # FIXME To be removed when CORE-17637 is resolved
    -Wno-maybe-uninitialized
)

if(ARCH STREQUAL "amd64")
    add_compile_options(-Wno-format)
elseif(ARCH STREQUAL "arm")
    add_compile_options(-Wno-attributes)
endif()

# Optimizations
# FIXME: Revisit this to see if we even need these levels
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O2 -DNDEBUG=)
    add_compile_options(-Wno-unused-variable)
    add_compile_options(-Wno-unused-but-set-variable)
else()
    if(OPTIMIZE STREQUAL "1")
        add_compile_options(-Os)
        if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
            add_compile_options(-ftracer)
        endif()
    elseif(OPTIMIZE STREQUAL "2")
        add_compile_options(-Os)
    elseif(OPTIMIZE STREQUAL "3")
        add_compile_options(-Og)
    elseif(OPTIMIZE STREQUAL "4")
        add_compile_options(-O1)
    elseif(OPTIMIZE STREQUAL "5")
        add_compile_options(-O2)
    elseif(OPTIMIZE STREQUAL "6")
        add_compile_options(-O3)
    elseif(OPTIMIZE STREQUAL "7")
        add_compile_options(-Ofast)
    endif()
endif()

# Link-time code generation
if(LTCG)
    add_compile_options(-flto -fno-fat-lto-objects)
endif()

if(ARCH STREQUAL "i386")
    add_compile_options(-fno-optimize-sibling-calls -fno-omit-frame-pointer -mstackrealign)
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
        add_compile_options(-mpreferred-stack-boundary=3 -fno-set-stack-executable)
    endif()
    # FIXME: this doesn't work. CMAKE_BUILD_TYPE is always "Debug"
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-momit-leaf-frame-pointer)
    endif()
elseif(ARCH STREQUAL "amd64")
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-mpreferred-stack-boundary=4)
    endif()
    add_compile_options(-Wno-error)
endif()

# Other
if(ARCH STREQUAL "amd64")
    add_compile_options(-mcx16) # Generate CMPXCHG16
    add_definitions(-U_X86_ -UWIN32)
elseif(ARCH STREQUAL "arm")
    add_definitions(-U_UNICODE -UUNICODE)
    add_definitions(-D__MSVCRT__) # DUBIOUS
endif()

# Fix build with GLIBCXX + our c++ headers
add_definitions(-D_GLIBCXX_HAVE_BROKEN_VSWPRINTF)

# Fix build with UCRT headers
add_definitions(-D_CRT_SUPPRESS_RESTRICT)

# Alternative arch name
if(ARCH STREQUAL "amd64")
    set(ARCH2 x86_64)
else()
    set(ARCH2 ${ARCH})
endif()

if(SEPARATE_DBG)
    # PDB style debug puts all dwarf debug info in a separate dbg file
    message(STATUS "Building separate debug symbols")
    file(MAKE_DIRECTORY ${REACTOS_BINARY_DIR}/symbols)
    if(CMAKE_GENERATOR STREQUAL "Ninja")
        # Those variables seems to be set but empty in newer CMake versions
        # and Ninja generator relies on them to generate PDB name, so unset them.
        unset(MSVC_C_ARCHITECTURE_ID)
        unset(MSVC_CXX_ARCHITECTURE_ID)
        set(CMAKE_DEBUG_SYMBOL_SUFFIX "")
        set(SYMBOL_FILE <TARGET_PDB>)
    else()
        set(SYMBOL_FILE <TARGET>)
    endif()

    if (NOT NO_ROSSYM)
        get_target_property(RSYM native-rsym IMPORTED_LOCATION)
        set(strip_debug "${RSYM} -s ${REACTOS_SOURCE_DIR} <TARGET> <TARGET>")
    else()
        set(strip_debug "${CMAKE_STRIP} --strip-debug <TARGET>")
    endif()

    set(CMAKE_C_LINK_EXECUTABLE
        "<CMAKE_C_COMPILER> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>"
        "${CMAKE_STRIP} --only-keep-debug <TARGET> -o ${REACTOS_BINARY_DIR}/symbols/${SYMBOL_FILE}"
        ${strip_debug})
    set(CMAKE_CXX_LINK_EXECUTABLE
        "<CMAKE_CXX_COMPILER> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>"
        "${CMAKE_STRIP} --only-keep-debug <TARGET> -o ${REACTOS_BINARY_DIR}/symbols/${SYMBOL_FILE}"
        ${strip_debug})
    set(CMAKE_C_CREATE_SHARED_LIBRARY
        "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>"
        "${CMAKE_STRIP} --only-keep-debug <TARGET> -o ${REACTOS_BINARY_DIR}/symbols/${SYMBOL_FILE}"
        ${strip_debug})
    set(CMAKE_CXX_CREATE_SHARED_LIBRARY
        "<CMAKE_CXX_COMPILER> <CMAKE_SHARED_LIBRARY_CXX_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>"
        "${CMAKE_STRIP} --only-keep-debug <TARGET> -o ${REACTOS_BINARY_DIR}/symbols/${SYMBOL_FILE}"
        ${strip_debug})
    set(CMAKE_RC_CREATE_SHARED_LIBRARY
        "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>"
        "${CMAKE_STRIP} --only-keep-debug <TARGET> -o ${REACTOS_BINARY_DIR}/symbols/${SYMBOL_FILE}"
        ${strip_debug})
elseif(NO_ROSSYM)
    # Dwarf-based build
    message(STATUS "Generating a dwarf-based build (no rsym)")
    set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> ${CMAKE_C_FLAGS} <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
    set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> ${CMAKE_CXX_FLAGS} <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
    set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> ${CMAKE_C_FLAGS} <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
    set(CMAKE_CXX_CREATE_SHARED_LIBRARY "<CMAKE_CXX_COMPILER> ${CMAKE_CXX_FLAGS} <CMAKE_SHARED_LIBRARY_CXX_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
    set(CMAKE_RC_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> ${CMAKE_C_FLAGS} <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
else()
    # Normal rsym build
    get_target_property(RSYM native-rsym IMPORTED_LOCATION)

    set(CMAKE_C_LINK_EXECUTABLE
        "<CMAKE_C_COMPILER> ${CMAKE_C_FLAGS} <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>"
        "${RSYM} -s ${REACTOS_SOURCE_DIR} <TARGET> <TARGET>")
    set(CMAKE_CXX_LINK_EXECUTABLE
        "<CMAKE_CXX_COMPILER> ${CMAKE_CXX_FLAGS} <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>"
        "${RSYM} -s ${REACTOS_SOURCE_DIR} <TARGET> <TARGET>")
    set(CMAKE_C_CREATE_SHARED_LIBRARY
        "<CMAKE_C_COMPILER> ${CMAKE_C_FLAGS} <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>"
        "${RSYM} -s ${REACTOS_SOURCE_DIR} <TARGET> <TARGET>")
    set(CMAKE_CXX_CREATE_SHARED_LIBRARY
        "<CMAKE_CXX_COMPILER> ${CMAKE_CXX_FLAGS} <CMAKE_SHARED_LIBRARY_CXX_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>"
        "${RSYM} -s ${REACTOS_SOURCE_DIR} <TARGET> <TARGET>")
    set(CMAKE_RC_CREATE_SHARED_LIBRARY
        "<CMAKE_C_COMPILER> ${CMAKE_C_FLAGS} <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
endif()

set(CMAKE_C_CREATE_SHARED_MODULE ${CMAKE_C_CREATE_SHARED_LIBRARY})
set(CMAKE_CXX_CREATE_SHARED_MODULE ${CMAKE_CXX_CREATE_SHARED_LIBRARY})
set(CMAKE_RC_CREATE_SHARED_MODULE ${CMAKE_RC_CREATE_SHARED_LIBRARY})

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS_INIT} -Wl,--disable-stdcall-fixup,--gc-sections")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS_INIT} -Wl,--disable-stdcall-fixup")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS_INIT} -Wl,--disable-stdcall-fixup")

set(CMAKE_C_COMPILE_OBJECT "<CMAKE_C_COMPILER> <DEFINES> ${_compress_debug_sections_flag} <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>")
# FIXME: Once the GCC toolchain bugs are fixed, add _compress_debug_sections_flag to CXX too
set(CMAKE_CXX_COMPILE_OBJECT "<CMAKE_CXX_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>")
set(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> ${_compress_debug_sections_flag} -x assembler-with-cpp -o <OBJECT> -I${REACTOS_SOURCE_DIR}/sdk/include/asm -I${REACTOS_BINARY_DIR}/sdk/include/asm <INCLUDES> <FLAGS> <DEFINES> -D__ASM__ -c <SOURCE>")

set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff <INCLUDES> <FLAGS> -DRC_INVOKED -D__WIN32__=1 -D__FLAT__=1 ${I18N_DEFS} <DEFINES> <SOURCE> <OBJECT>")

if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(RC_PREPROCESSOR_TARGET "--preprocessor-arg=--target=${CMAKE_C_COMPILER_TARGET}")
else()
    set(RC_PREPROCESSOR_TARGET "")
endif()

# We have to pass args to windres. one... by... one...
set(CMAKE_DEPFILE_FLAGS_RC "--preprocessor=\"${CMAKE_C_COMPILER}\" ${RC_PREPROCESSOR_TARGET} --preprocessor-arg=-E --preprocessor-arg=-nostdinc --preprocessor-arg=-xc-header --preprocessor-arg=-MMD --preprocessor-arg=-MF --preprocessor-arg=<DEPFILE> --preprocessor-arg=-MT --preprocessor-arg=<OBJECT>")

# Optional 3rd parameter: stdcall stack bytes
function(set_entrypoint MODULE ENTRYPOINT)
    if(${ENTRYPOINT} STREQUAL "0")
        target_link_options(${MODULE} PRIVATE "-Wl,-entry,0")
    elseif(ARCH STREQUAL "i386")
        set(_entrysymbol _${ENTRYPOINT})
        if(${ARGC} GREATER 2)
            set(_entrysymbol ${_entrysymbol}@${ARGV2})
        endif()
        target_link_options(${MODULE} PRIVATE "-Wl,-entry,${_entrysymbol}")
    else()
        target_link_options(${MODULE} PRIVATE "-Wl,-entry,${ENTRYPOINT}")
    endif()
endfunction()

function(set_subsystem MODULE SUBSYSTEM)
    target_link_options(${MODULE} PRIVATE "-Wl,--subsystem,${SUBSYSTEM}:5.01")
endfunction()

function(set_image_base MODULE IMAGE_BASE)
    target_link_options(${MODULE} PRIVATE "-Wl,--image-base,${IMAGE_BASE}")
endfunction()

function(set_module_type_toolchain MODULE TYPE)
    # Set the PE image version numbers from the NT OS version ReactOS is based on
    target_link_options(${MODULE} PRIVATE
        -Wl,--major-image-version,5 -Wl,--minor-image-version,01 -Wl,--major-os-version,5 -Wl,--minor-os-version,01)

    if(TYPE IN_LIST KERNEL_MODULE_TYPES)
        target_link_options(${MODULE} PRIVATE -Wl,--exclude-all-symbols,-file-alignment=0x1000,-section-alignment=0x1000)

        if(${TYPE} STREQUAL "wdmdriver")
            target_link_options(${MODULE} PRIVATE "-Wl,--wdmdriver")
        endif()

        # Place INIT &.rsrc section at the tail of the module, before .reloc
        add_linker_script(${MODULE} ${REACTOS_SOURCE_DIR}/sdk/cmake/init-section.lds)

        # Fixup section characteristics
        #  - Remove flags that LD overzealously puts (alignment flag, Initialized flags for code sections)
        #  - INIT section is made discardable
        #  - .rsrc is made read-only and discardable
        #  - PAGE & .edata sections are made pageable.
        add_custom_command(TARGET ${MODULE} POST_BUILD
            COMMAND native-pefixup --${TYPE} $<TARGET_FILE:${MODULE}>)

        # Believe it or not, cmake doesn't do that
        set_property(TARGET ${MODULE} APPEND PROPERTY LINK_DEPENDS $<TARGET_PROPERTY:native-pefixup,IMPORTED_LOCATION>)
    endif()
endfunction()

function(add_delay_importlibs _module)
    get_target_property(_module_type ${_module} TYPE)
    if(_module_type STREQUAL "STATIC_LIBRARY")
        message(FATAL_ERROR "Cannot add delay imports to a static library")
    endif()
    foreach(_lib ${ARGN})
        get_filename_component(_basename "${_lib}" NAME_WE)
        target_link_libraries(${_module} lib${_basename}_delayed)
    endforeach()
    target_link_libraries(${_module} delayimp)
endfunction()

if(NOT ARCH STREQUAL "i386")
    set(DECO_OPTION "-@")
endif()

function(fixup_load_config _target)
    add_custom_command(TARGET ${_target} POST_BUILD
        COMMAND native-pefixup --loadconfig "$<TARGET_FILE:${_target}>"
        COMMENT "Patching in LOAD_CONFIG"
        DEPENDS native-pefixup)
endfunction()

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR
   CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(__spec2def_dbg_arg "--dbg")
endif()

function(generate_import_lib _libname _dllname _spec_file __version_arg)
    # Generate the def for the import lib
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_libname}_implib.def
        COMMAND native-spec2def ${__version_arg} ${__spec2def_dbg_arg} -n=${_dllname} -a=${ARCH2} ${ARGN} --implib -d=${CMAKE_CURRENT_BINARY_DIR}/${_libname}_implib.def ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file} native-spec2def)

    # With this, we let DLLTOOL create an import library
    set(LIBRARY_PRIVATE_DIR ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${_libname}.dir)
    add_custom_command(
        OUTPUT ${LIBRARY_PRIVATE_DIR}/${_libname}.a
        # ar just puts stuff into the archive, without looking twice. Just delete the lib, we're going to rebuild it anyway
        COMMAND ${CMAKE_COMMAND} -E rm -f $<TARGET_FILE:${_libname}>
        COMMAND ${CMAKE_DLLTOOL} --def ${CMAKE_CURRENT_BINARY_DIR}/${_libname}_implib.def --kill-at --output-lib=${_libname}.a -t ${_libname}
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_libname}_implib.def
        WORKING_DIRECTORY ${LIBRARY_PRIVATE_DIR})

    # We create a static library with the importlib thus created as object. AR will extract the obj files and archive it again as a thin lib
    set_source_files_properties(
        ${LIBRARY_PRIVATE_DIR}/${_libname}.a
        PROPERTIES
        EXTERNAL_OBJECT TRUE)
    _add_library(${_libname} STATIC EXCLUDE_FROM_ALL
        ${LIBRARY_PRIVATE_DIR}/${_libname}.a)
    set_target_properties(${_libname}
        PROPERTIES
        LINKER_LANGUAGE "C"
        PREFIX "")

    # Do the same with delay-import libs
    set(LIBRARY_PRIVATE_DIR ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${_libname}_delayed.dir)
    add_custom_command(
        OUTPUT ${LIBRARY_PRIVATE_DIR}/${_libname}_delayed.a
        # ar just puts stuff into the archive, without looking twice. Just delete the lib, we're going to rebuild it anyway
        COMMAND ${CMAKE_COMMAND} -E rm -f $<TARGET_FILE:${_libname}_delayed>
        COMMAND ${CMAKE_DLLTOOL} --def ${CMAKE_CURRENT_BINARY_DIR}/${_libname}_implib.def --kill-at --output-delaylib=${_libname}_delayed.a -t ${_libname}_delayed
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_libname}_implib.def
        WORKING_DIRECTORY ${LIBRARY_PRIVATE_DIR})

    # We create a static library with the importlib thus created. AR will extract the obj files and archive it again as a thin lib
    set_source_files_properties(
        ${LIBRARY_PRIVATE_DIR}/${_libname}_delayed.a
        PROPERTIES
        EXTERNAL_OBJECT TRUE)
    _add_library(${_libname}_delayed STATIC EXCLUDE_FROM_ALL
        ${LIBRARY_PRIVATE_DIR}/${_libname}_delayed.a)
    set_target_properties(${_libname}_delayed
        PROPERTIES
        LINKER_LANGUAGE "C"
        PREFIX "")
endfunction()

function(spec2def _dllname _spec_file)

    cmake_parse_arguments(__spec2def "ADD_IMPORTLIB;NO_PRIVATE_WARNINGS;WITH_RELAY" "VERSION" "" ${ARGN})

    # Get library basename
    get_filename_component(_file ${_dllname} NAME_WE)

    # Error out on anything else than spec
    if(NOT ${_spec_file} MATCHES ".*\\.spec")
        message(FATAL_ERROR "spec2def only takes spec files as input.")
    endif()

    if(__spec2def_WITH_RELAY)
        set(__with_relay_arg "--with-tracing")
    endif()

    if(__spec2def_VERSION)
        set(__version_arg "--version=0x${__spec2def_VERSION}")
    else()
        set(__version_arg "--version=${DLL_EXPORT_VERSION}")
    endif()

    # Generate exports def and C stubs file for the DLL
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_file}.def ${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c
        COMMAND native-spec2def -n=${_dllname} -a=${ARCH2} -d=${CMAKE_CURRENT_BINARY_DIR}/${_file}.def -s=${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c ${__with_relay_arg} ${__version_arg} ${__spec2def_dbg_arg} ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file} native-spec2def)

    # Do not use precompiled headers for the stub file
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

    if(__spec2def_ADD_IMPORTLIB)
        set(_extraflags)
        if(__spec2def_NO_PRIVATE_WARNINGS)
            set(_extraflags --no-private-warnings)
        endif()

        generate_import_lib(lib${_file} ${_dllname} ${_spec_file} ${_extraflags} "${__version_arg}")
    endif()
endfunction()

macro(macro_mc FLAG FILE)
    set(COMMAND_MC ${CMAKE_MC_COMPILER} -u ${FLAG} -b -h ${CMAKE_CURRENT_BINARY_DIR}/ -r ${CMAKE_CURRENT_BINARY_DIR}/ ${FILE})
endmacro()

# PSEH lib, needed with mingw
set(PSEH_LIB "pseh")

function(CreateBootSectorTarget _target_name _asm_file _binary_file _base_address)
    set(_object_file ${_binary_file}.o)

    get_defines(_defines)
    get_includes(_includes)

    add_custom_command(
        OUTPUT ${_object_file}
        COMMAND ${CMAKE_ASM_COMPILER} -x assembler-with-cpp -o ${_object_file} -I${REACTOS_SOURCE_DIR}/sdk/include/asm -I${REACTOS_BINARY_DIR}/sdk/include/asm ${_includes} ${_defines} -D__ASM__ -c ${_asm_file}
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
    # We don't allow warnings in trunk, this needs to be reworked. See CORE-6959.
    #target_compile_options(${__module} PRIVATE "-Wno-error")
endfunction()

function(convert_asm_file _source_file _target_file)
    get_filename_component(_source_file_base_name ${_source_file} NAME_WE)
    get_filename_component(_source_file_full_path ${_source_file} ABSOLUTE)
    set(_preprocessed_asm_file ${CMAKE_CURRENT_BINARY_DIR}/${_target_file})
    add_custom_command(
        OUTPUT ${_preprocessed_asm_file}
        COMMAND native-asmpp ${_source_file_full_path} > ${_preprocessed_asm_file}
        DEPENDS native-asmpp ${_source_file_full_path})

endfunction()

function(convert_asm_files)
    foreach(_source_file ${ARGN})
        convert_asm_file(${_source_file} ${_source_file}.s)
    endforeach()
endfunction()

macro(add_asm_files _target)
    foreach(_source_file ${ARGN})
        get_filename_component(_extension ${_source_file} EXT)
        get_filename_component(_source_file_base_name ${_source_file} NAME_WE)
        if (${_extension} STREQUAL ".asm")
            convert_asm_file(${_source_file} ${_source_file}.s)
            list(APPEND ${_target} ${CMAKE_CURRENT_BINARY_DIR}/${_source_file}.s)
        elseif (${_extension} STREQUAL ".inc")
            convert_asm_file(${_source_file} ${_source_file}.h)
            list(APPEND ${_target} ${CMAKE_CURRENT_BINARY_DIR}/${_source_file}.h)
        else()
            list(APPEND ${_target} ${_source_file})
        endif()
    endforeach()
endmacro()

function(add_linker_script _target _linker_script_file)
    get_filename_component(_file_full_path ${_linker_script_file} ABSOLUTE)
    target_link_options(${_target} PRIVATE "-Wl,-T,${_file_full_path}")
    set_property(TARGET ${_target} APPEND PROPERTY LINK_DEPENDS ${_file_full_path})
endfunction()

# Manage our C++ options
# we disable standard includes if we don't use the STL
add_compile_options("$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<IN_LIST:cppstl,$<TARGET_PROPERTY:LINK_LIBRARIES>>>>:-nostdinc>")
# we disable RTTI, unless said so
add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:$<IF:$<BOOL:$<TARGET_PROPERTY:WITH_CXX_RTTI>>,-frtti,-fno-rtti>>")
# We disable exceptions, unless said so
add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:$<IF:$<BOOL:$<TARGET_PROPERTY:WITH_CXX_EXCEPTIONS>>,-fexceptions,-fno-exceptions>>")

# G++ shipped with ROSBE uses sjlj exceptions on i386. Tell Clang it is so
if(CMAKE_C_COMPILER_ID STREQUAL "Clang" AND ARCH STREQUAL "i386")
    add_compile_options("$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<BOOL:$<TARGET_PROPERTY:WITH_CXX_EXCEPTIONS>>>:-fsjlj-exceptions>")
endif()

# Find default G++ libraries
if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(GXX_EXECUTABLE ${CMAKE_CXX_COMPILER_TARGET}-g++)
else()
    set(GXX_EXECUTABLE ${CMAKE_CXX_COMPILER})
endif()

execute_process(COMMAND ${GXX_EXECUTABLE} -print-file-name=libwinpthread.a OUTPUT_VARIABLE LIBWINPTHREAD_LOCATION)
if(LIBWINPTHREAD_LOCATION MATCHES "mingw32")
    add_library(libwinpthread STATIC IMPORTED)
    string(STRIP ${LIBWINPTHREAD_LOCATION} LIBWINPTHREAD_LOCATION)
    message(STATUS "Using libwinpthread from ${LIBWINPTHREAD_LOCATION}")
    set_target_properties(libwinpthread PROPERTIES IMPORTED_LOCATION ${LIBWINPTHREAD_LOCATION})
    # libwinpthread needs kernel32 imports, a CRT and msvcrtex
    target_link_libraries(libwinpthread INTERFACE libkernel32 libmsvcrt msvcrtex)
else()
    add_library(libwinpthread INTERFACE)
endif()

add_library(libgcc STATIC IMPORTED)
execute_process(COMMAND ${GXX_EXECUTABLE} -print-file-name=libgcc.a OUTPUT_VARIABLE LIBGCC_LOCATION)
string(STRIP ${LIBGCC_LOCATION} LIBGCC_LOCATION)
set_target_properties(libgcc PROPERTIES IMPORTED_LOCATION ${LIBGCC_LOCATION})
# libgcc needs kernel32 and winpthread (an appropriate CRT must be linked manually)
target_link_libraries(libgcc INTERFACE libwinpthread libkernel32)

add_library(libsupc++ STATIC IMPORTED GLOBAL)
execute_process(COMMAND ${GXX_EXECUTABLE} -print-file-name=libsupc++.a OUTPUT_VARIABLE LIBSUPCXX_LOCATION)
string(STRIP ${LIBSUPCXX_LOCATION} LIBSUPCXX_LOCATION)
set_target_properties(libsupc++ PROPERTIES IMPORTED_LOCATION ${LIBSUPCXX_LOCATION})
# libsupc++ requires libgcc and stdc++compat
target_link_libraries(libsupc++ INTERFACE libgcc stdc++compat)

add_library(libmingwex STATIC IMPORTED)
execute_process(COMMAND ${GXX_EXECUTABLE} -print-file-name=libmingwex.a OUTPUT_VARIABLE LIBMINGWEX_LOCATION)
string(STRIP ${LIBMINGWEX_LOCATION} LIBMINGWEX_LOCATION)
set_target_properties(libmingwex PROPERTIES IMPORTED_LOCATION ${LIBMINGWEX_LOCATION})
# libmingwex requires a CRT and imports from kernel32
target_link_libraries(libmingwex INTERFACE libmsvcrt libkernel32)

add_library(libstdc++ STATIC IMPORTED GLOBAL)
execute_process(COMMAND ${GXX_EXECUTABLE} -print-file-name=libstdc++.a OUTPUT_VARIABLE LIBSTDCCXX_LOCATION)
string(STRIP ${LIBSTDCCXX_LOCATION} LIBSTDCCXX_LOCATION)
set_target_properties(libstdc++ PROPERTIES IMPORTED_LOCATION ${LIBSTDCCXX_LOCATION})
# libstdc++ requires libsupc++ and mingwex provided by GCC
target_link_libraries(libstdc++ INTERFACE libsupc++ libmingwex oldnames)
# this is for our SAL annotations
target_compile_definitions(libstdc++ INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:PAL_STDCPP_COMPAT>")

# Create our alias libraries
add_library(cppstl ALIAS libstdc++)

