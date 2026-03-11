
# =============================================================================
# Clang/LLVM configuration for ReactOS (clang19+)
# =============================================================================

if(_REACTOS_CLANG_BUILD_CONFIG)

# =============================================================================
# Build Configuration (included from CMakeLists.txt after project())
# =============================================================================

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

# Clang builds always use dwarf (no rsym)
set(NO_ROSSYM TRUE)

# Clang supports native SEH (__try/__except) on all architectures,
# so PSEH3 (x86 GCC-only) is never needed.
set(USE_PSEH3 0)
add_compile_options(-U_USE_PSEH3)

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
add_compile_options(-pipe -fms-extensions -fno-strict-aliasing -fno-common)

if(ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64")
    add_compile_options(-mlong-double-64)
endif()

# Prevent Clang from searching system library include directories, but keep
# Clang's builtin includes (intrinsics headers like xmmintrin.h, etc.)
# Use -nostdlibinc instead of -nostdinc so Clang's resource dir includes are preserved.
# The case for C++ is handled through the reactos_c++ INTERFACE library
add_compile_options("$<$<NOT:$<COMPILE_LANGUAGE:CXX>>:-nostdlibinc>")

# Add Clang's builtin include path (intrinsics headers like xmmintrin.h, mmintrin.h, etc.)
# before everything else. ReactOS ships its own versions of these in sdk/include/vcruntime
# that use GCC-specific __builtin_ia32_* builtins which don't exist in Clang.
# Injected into CMAKE_C/CXX_COMPILE_OBJECT before <INCLUDES> so Clang's own
# intrinsics headers are found before ReactOS's GCC-specific versions.
execute_process(COMMAND ${CMAKE_C_COMPILER} -print-resource-dir OUTPUT_VARIABLE CLANG_RESOURCE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)

# Clang-specific options
add_compile_options("$<$<COMPILE_LANGUAGE:C>:-Wno-microsoft>")
add_compile_options(-Wno-pragma-pack)
add_compile_options(-fno-associative-math)

# disable "libcall optimization"
# see https://mudongliang.github.io/2020/12/02/undefined-reference-to-stpcpy.html
add_compile_options(-fno-builtin-stpcpy)

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

# Debugging
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    if(SEPARATE_DBG)
        add_compile_options(-gdwarf-2 -ggdb)
    else()
        add_compile_options(-gdwarf-2 -gstrict-dwarf)
    endif()
endif()

# Tuning
add_compile_options(-march=${OARCH} -mtune=${TUNE})

# Warnings, errors
# Clang builds don't use -Werror
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
    -Wno-format
    -Wno-error=implicit-function-declaration
    -Wno-error=incompatible-library-redeclaration
)

if(ARCH STREQUAL "arm")
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
    add_compile_options(-flto)
endif()

if(ARCH STREQUAL "i386")
    add_compile_options(-fno-optimize-sibling-calls -fno-omit-frame-pointer -mstackrealign)
    add_compile_options("$<$<COMPILE_LANGUAGE:ASM>:-fno-integrated-as>")
    add_compile_options("$<$<COMPILE_LANGUAGE:ASM>:-U__clang__>")
    # FIXME: this doesn't work. CMAKE_BUILD_TYPE is always "Debug"
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-momit-leaf-frame-pointer)
    endif()
elseif(ARCH STREQUAL "amd64")
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

# llvm-dlltool machine flag
if(ARCH STREQUAL "i386")
    set(LLVM_DLLTOOL_MACHINE i386)
elseif(ARCH STREQUAL "amd64")
    set(LLVM_DLLTOOL_MACHINE i386:x86-64)
elseif(ARCH STREQUAL "arm")
    set(LLVM_DLLTOOL_MACHINE arm)
elseif(ARCH STREQUAL "arm64")
    set(LLVM_DLLTOOL_MACHINE arm64)
endif()

# GNU binutils/GCC cross packages use the target processor from the toolchain triplet
set(_gnu_tool_triplet ${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32)

# GNU dlltool from binutils-mingw (needed for delay-import libs which llvm-dlltool doesn't support)
find_program(GNU_DLLTOOL ${_gnu_tool_triplet}-dlltool)
if(NOT GNU_DLLTOOL)
    message(WARNING "GNU dlltool (${_gnu_tool_triplet}-dlltool) not found. Delay-import libraries will fail to build.")
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

    set(strip_debug "${CMAKE_STRIP} --strip-debug <TARGET>")

    set(CMAKE_C_LINK_EXECUTABLE
        "<CMAKE_C_COMPILER> -Wl,--start-group <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -Wl,--end-group"
        "${CMAKE_STRIP} --only-keep-debug <TARGET> -o ${REACTOS_BINARY_DIR}/symbols/${SYMBOL_FILE}"
        ${strip_debug})
    set(CMAKE_CXX_LINK_EXECUTABLE
        "<CMAKE_CXX_COMPILER> -Wl,--start-group <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -Wl,--end-group"
        "${CMAKE_STRIP} --only-keep-debug <TARGET> -o ${REACTOS_BINARY_DIR}/symbols/${SYMBOL_FILE}"
        ${strip_debug})
    set(CMAKE_C_CREATE_SHARED_LIBRARY
        "<CMAKE_C_COMPILER> -Wl,--start-group <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -Wl,--end-group"
        "${CMAKE_STRIP} --only-keep-debug <TARGET> -o ${REACTOS_BINARY_DIR}/symbols/${SYMBOL_FILE}"
        ${strip_debug})
    set(CMAKE_CXX_CREATE_SHARED_LIBRARY
        "<CMAKE_CXX_COMPILER> -Wl,--start-group <CMAKE_SHARED_LIBRARY_CXX_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -Wl,--end-group"
        "${CMAKE_STRIP} --only-keep-debug <TARGET> -o ${REACTOS_BINARY_DIR}/symbols/${SYMBOL_FILE}"
        ${strip_debug})
    set(CMAKE_RC_CREATE_SHARED_LIBRARY
        "<CMAKE_C_COMPILER> -Wl,--start-group <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -Wl,--end-group"
        "${CMAKE_STRIP} --only-keep-debug <TARGET> -o ${REACTOS_BINARY_DIR}/symbols/${SYMBOL_FILE}"
        ${strip_debug})
else()
    # Dwarf-based build (always for Clang, no rsym)
    message(STATUS "Generating a dwarf-based build (no rsym)")
    set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> -Wl,--start-group ${CMAKE_C_FLAGS} <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -Wl,--end-group")
    set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> -Wl,--start-group ${CMAKE_CXX_FLAGS} <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -Wl,--end-group")
    set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> -Wl,--start-group ${CMAKE_C_FLAGS} <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -Wl,--end-group")
    set(CMAKE_CXX_CREATE_SHARED_LIBRARY "<CMAKE_CXX_COMPILER> -Wl,--start-group ${CMAKE_CXX_FLAGS} <CMAKE_SHARED_LIBRARY_CXX_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -Wl,--end-group")
    set(CMAKE_RC_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> -Wl,--start-group ${CMAKE_C_FLAGS} <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -Wl,--end-group")
endif()

set(CMAKE_C_CREATE_SHARED_MODULE ${CMAKE_C_CREATE_SHARED_LIBRARY})
set(CMAKE_CXX_CREATE_SHARED_MODULE ${CMAKE_CXX_CREATE_SHARED_LIBRARY})
set(CMAKE_RC_CREATE_SHARED_MODULE ${CMAKE_RC_CREATE_SHARED_LIBRARY})

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS_INIT} -Wl,--disable-stdcall-fixup,--gc-sections")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS_INIT} -Wl,--disable-stdcall-fixup")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS_INIT} -Wl,--disable-stdcall-fixup")

set(CMAKE_C_COMPILE_OBJECT "<CMAKE_C_COMPILER> <DEFINES> ${_compress_debug_sections_flag} -I${CLANG_RESOURCE_DIR}/include <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>")
set(CMAKE_CXX_COMPILE_OBJECT "<CMAKE_CXX_COMPILER> <DEFINES> -I${CLANG_RESOURCE_DIR}/include <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>")
set(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> ${_compress_debug_sections_flag} -x assembler-with-cpp -o <OBJECT> -I${REACTOS_SOURCE_DIR}/sdk/include/asm -I${REACTOS_BINARY_DIR}/sdk/include/asm <INCLUDES> <FLAGS> <DEFINES> -D__ASM__ -c <SOURCE>")

set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff <INCLUDES> <FLAGS> -DRC_INVOKED -D__WIN32__=1 -D__FLAT__=1 ${I18N_DEFS} <DEFINES> <SOURCE> <OBJECT>")

set(RC_PREPROCESSOR_TARGET "--preprocessor-arg=--target=${CMAKE_C_COMPILER_TARGET}")

# We have to pass args to windres. one... by... one...
set(CMAKE_DEPFILE_FLAGS_RC "--preprocessor=\"${CMAKE_C_COMPILER}\" ${RC_PREPROCESSOR_TARGET} --preprocessor-arg=-E --preprocessor-arg=-nostdlibinc --preprocessor-arg=-xc-header --preprocessor-arg=-MMD --preprocessor-arg=-MF --preprocessor-arg=<DEPFILE> --preprocessor-arg=-MT --preprocessor-arg=<OBJECT>")

# Optional 3rd parameter: stdcall stack bytes
function(set_entrypoint MODULE ENTRYPOINT)
    if(${ENTRYPOINT} STREQUAL "0")
        # lld treats -entry,0 as symbol name "0". Use --entry= (empty) to set entry point to address 0.
        target_link_options(${MODULE} PRIVATE "-Wl,--entry=")
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

    # Clang's _setjmp builtin on x64 lowers to __intrinsic_setjmp which must be
    # statically linked (it captures the caller's frame, so a DLL export won't work).
    target_link_libraries(${MODULE} setjmp)

    if(TYPE IN_LIST KERNEL_MODULE_TYPES)
        if(${TYPE} STREQUAL "kmdfdriver")
            set(TYPE "wdmdriver")
        endif()

        target_link_options(${MODULE} PRIVATE -Wl,--exclude-all-symbols,-file-alignment=0x1000,-section-alignment=0x1000)

        # Note: GNU ld supports --wdmdriver, but LLD does not.
        # The WDM flag is set by pefixup in the POST_BUILD step instead.

        # Place INIT &.rsrc section at the tail of the module, before .reloc
        add_linker_script(${MODULE} ${REACTOS_SOURCE_DIR}/sdk/cmake/init-section.lds)

        add_custom_command(TARGET ${MODULE} POST_BUILD
            COMMAND native-pefixup --${TYPE} $<TARGET_FILE:${MODULE}>)

        set_property(TARGET ${MODULE} APPEND PROPERTY LINK_DEPENDS $<TARGET_PROPERTY:native-pefixup,IMPORTED_LOCATION>)
    endif()
endfunction()

function(add_delay_importlibs _module)
    get_target_property(_module_type ${_module} TYPE)
    if(_module_type STREQUAL "STATIC_LIBRARY")
        message(FATAL_ERROR "Cannot add delay imports to a static library")
    endif()
    # LLD doesn't merge dlltool-generated .didat$N subsections correctly,
    # placing the delay-load IAT in read-only .rdata and causing access violations.
    # Use regular import libs + LLD's native --delayload flag instead.
    foreach(_lib ${ARGN})
        get_filename_component(_basename "${_lib}" NAME_WE)
        target_link_libraries(${_module} lib${_basename})
        # Determine the DLL filename
        if("${_lib}" MATCHES "\\.")
            set(_dllname "${_lib}")
        else()
            set(_dllname "${_lib}.dll")
        endif()
        target_link_options(${_module} PRIVATE "-Wl,--delayload,${_dllname}")
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

function(generate_import_lib _libname _dllname _spec_file __version_arg __dbg_arg)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_libname}_implib.def
        COMMAND native-spec2def ${__version_arg} ${__dbg_arg} -n=${_dllname} -a=${ARCH2} ${ARGN} --implib -d=${CMAKE_CURRENT_BINARY_DIR}/${_libname}_implib.def ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file} native-spec2def)

    # llvm-dlltool uses short flags: -d (def), -k (kill-at), -l (output-lib), -m (machine)
    set(LIBRARY_PRIVATE_DIR ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${_libname}.dir)
    add_custom_command(
        OUTPUT ${LIBRARY_PRIVATE_DIR}/${_libname}.a
        COMMAND ${CMAKE_COMMAND} -E rm -f ${LIBRARY_PRIVATE_DIR}/${_libname}.a
        COMMAND ${CMAKE_DLLTOOL} -d ${CMAKE_CURRENT_BINARY_DIR}/${_libname}_implib.def -k -l ${_libname}.a -m ${LLVM_DLLTOOL_MACHINE} -t ${_libname}
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_libname}_implib.def
        WORKING_DIRECTORY ${LIBRARY_PRIVATE_DIR})

    add_custom_target(${_libname}_implib_target DEPENDS ${LIBRARY_PRIVATE_DIR}/${_libname}.a)

    _add_library(${_libname} STATIC IMPORTED GLOBAL)
    set_target_properties(${_libname} PROPERTIES IMPORTED_LOCATION ${LIBRARY_PRIVATE_DIR}/${_libname}.a)
    add_dependencies(${_libname} ${_libname}_implib_target)

    # llvm-dlltool does not support --output-delaylib, use GNU dlltool from binutils for delay-import
    set(LIBRARY_PRIVATE_DIR ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${_libname}_delayed.dir)
    add_custom_command(
        OUTPUT ${LIBRARY_PRIVATE_DIR}/${_libname}_delayed.a
        COMMAND ${CMAKE_COMMAND} -E rm -f ${LIBRARY_PRIVATE_DIR}/${_libname}_delayed.a
        COMMAND ${GNU_DLLTOOL} --def ${CMAKE_CURRENT_BINARY_DIR}/${_libname}_implib.def --kill-at --output-delaylib=${_libname}_delayed.a -t ${_libname}_delayed
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_libname}_implib.def
        WORKING_DIRECTORY ${LIBRARY_PRIVATE_DIR})

    add_custom_target(${_libname}_delayed_implib_target DEPENDS ${LIBRARY_PRIVATE_DIR}/${_libname}_delayed.a)

    _add_library(${_libname}_delayed STATIC IMPORTED GLOBAL)
    set_target_properties(${_libname}_delayed PROPERTIES IMPORTED_LOCATION ${LIBRARY_PRIVATE_DIR}/${_libname}_delayed.a)
    add_dependencies(${_libname}_delayed ${_libname}_delayed_implib_target)
endfunction()

function(spec2def _dllname _spec_file)
    cmake_parse_arguments(__spec2def "ADD_IMPORTLIB;NO_PRIVATE_WARNINGS;WITH_RELAY;WITH_DBG;NO_DBG" "VERSION" "" ${ARGN})

    get_filename_component(_file ${_dllname} NAME_WLE)

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

    if(__spec2def_WITH_DBG OR (DBG AND NOT __spec2def_NO_DBG))
        set(__dbg_arg "--dbg")
    endif()

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_file}.def ${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c
        COMMAND native-spec2def -n=${_dllname} -a=${ARCH2} -d=${CMAKE_CURRENT_BINARY_DIR}/${_file}.def -s=${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c ${__with_relay_arg} ${__version_arg} ${__dbg_arg} ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file} native-spec2def)

    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${_file}_stubs.c PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

    if(__spec2def_ADD_IMPORTLIB)
        set(_extraflags)
        if(__spec2def_NO_PRIVATE_WARNINGS)
            set(_extraflags --no-private-warnings)
        endif()

        generate_import_lib(lib${_file} ${_dllname} ${_spec_file} ${_extraflags} "${__version_arg}" "${__dbg_arg}")
    endif()
endfunction()

macro(macro_mc FLAG FILE)
    set(COMMAND_MC ${CMAKE_MC_COMPILER} -u ${FLAG} -b -h ${CMAKE_CURRENT_BINARY_DIR}/ -r ${CMAKE_CURRENT_BINARY_DIR}/ ${FILE})
endmacro()

# PSEH lib, needed with mingw
set(PSEH_LIB "pseh")

function(CreateBootSectorTarget _target_name _asm_file _binary_file _base_address)
    set(_object_file ${_binary_file}.o)
    set(_preprocessed_file ${_binary_file}.pp.s)

    get_defines(_defines)
    get_includes(_includes)

    # Boot sector files are 16-bit real mode assembly with constructs that Clang's
    # integrated assembler doesn't support (symbol arithmetic, .org, etc.).
    # Use Clang for preprocessing only (-U__clang__ so GAS-compatible macros are used),
    # then GNU as for assembly, then obj2bin for binary extraction (COFF input).
    find_program(_gnu_as x86_64-w64-mingw32-as)
    if(NOT _gnu_as)
        find_program(_gnu_as as)
    endif()

    # Step 1: Preprocess with Clang (undefine __clang__ so asm.inc emits GAS-compatible macros)
    add_custom_command(
        OUTPUT ${_preprocessed_file}
        COMMAND ${CMAKE_ASM_COMPILER} -E -U__clang__ -x assembler-with-cpp -o ${_preprocessed_file} -I${REACTOS_SOURCE_DIR}/sdk/include/asm -I${REACTOS_BINARY_DIR}/sdk/include/asm ${_includes} ${_defines} -D__ASM__ ${_asm_file}
        DEPENDS ${_asm_file})

    # Step 2: Assemble with GNU as (produces COFF objects)
    add_custom_command(
        OUTPUT ${_object_file}
        COMMAND ${_gnu_as} ${_preprocessed_file} -o ${_object_file}
        DEPENDS ${_preprocessed_file})

    # Step 3: Extract binary with obj2bin (handles COFF relocations at base address)
    add_custom_command(
        OUTPUT ${_binary_file}
        COMMAND native-obj2bin ${_object_file} ${_binary_file} ${_base_address}
        DEPENDS ${_object_file} native-obj2bin)

    set_source_files_properties(${_object_file} ${_binary_file} PROPERTIES GENERATED TRUE)

    add_custom_target(${_target_name} ALL DEPENDS ${_binary_file})
endfunction()

function(add_asm16_bin _target _binary_file _base_address)
    set(_concatenated_asm_file ${CMAKE_CURRENT_BINARY_DIR}/${_target}.asm)
    set(_preprocessed_file ${CMAKE_CURRENT_BINARY_DIR}/${_target}.pp.s)
    set(_object_file ${CMAKE_CURRENT_BINARY_DIR}/${_target}.o)

    get_defines(_directory_defines)
    get_includes(_directory_includes)
    get_directory_property(_defines COMPILE_DEFINITIONS)

    foreach(_source_file ${ARGN})
        get_filename_component(_source_file_full_path ${_source_file} ABSOLUTE)
        get_source_file_property(_defines_semicolon_list ${_source_file_full_path} COMPILE_DEFINITIONS)
        foreach(_define ${_defines_semicolon_list})
            if(NOT ${_define} STREQUAL "NOTFOUND")
                list(APPEND _source_file_defines -D${_define})
            endif()
        endforeach()
        list(APPEND _source_file_list ${_source_file_full_path})
    endforeach()

    concatenate_files(${_concatenated_asm_file} ${_source_file_list})
    set_source_files_properties(${_concatenated_asm_file} PROPERTIES GENERATED TRUE)

    # 16-bit ASM: same 3-step approach as CreateBootSectorTarget
    find_program(_gnu_as x86_64-w64-mingw32-as)
    if(NOT _gnu_as)
        find_program(_gnu_as as)
    endif()

    # Step 1: Preprocess with Clang
    add_custom_command(
        OUTPUT ${_preprocessed_file}
        COMMAND ${CMAKE_ASM_COMPILER} -E -U__clang__ -x assembler-with-cpp -o ${_preprocessed_file} -I${REACTOS_SOURCE_DIR}/sdk/include/asm -I${REACTOS_BINARY_DIR}/sdk/include/asm ${_directory_includes} ${_source_file_defines} ${_directory_defines} -D__ASM__ ${_concatenated_asm_file}
        DEPENDS ${_concatenated_asm_file})

    # Step 2: Assemble with GNU as (produces COFF objects)
    add_custom_command(
        OUTPUT ${_object_file}
        COMMAND ${_gnu_as} ${_preprocessed_file} -o ${_object_file}
        DEPENDS ${_preprocessed_file})

    # Step 3: Extract binary with obj2bin
    add_custom_command(
        OUTPUT ${_binary_file}
        COMMAND native-obj2bin ${_object_file} ${_binary_file} ${_base_address}
        DEPENDS ${_object_file} native-obj2bin)

    add_custom_target(${_target} ALL DEPENDS ${_binary_file})
    set_target_properties(${_target} PROPERTIES BINARY_PATH ${_binary_file})
    add_clean_target(${_target})
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
    # lld does not support linker scripts in PE/COFF mode.
    # Linker script functionality must be handled via lld-compatible flags.
    # This function is intentionally a no-op for Clang/lld builds.
endfunction()

# Manage our C++ options
# we disable standard includes if we don't use the STL
add_compile_options("$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<IN_LIST:cppstl,$<TARGET_PROPERTY:LINK_LIBRARIES>>>>:-nostdlibinc>")
# we disable RTTI, unless said so
add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:$<IF:$<BOOL:$<TARGET_PROPERTY:WITH_CXX_RTTI>>,-frtti,-fno-rtti>>")
# We disable exceptions, unless said so
add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:$<IF:$<BOOL:$<TARGET_PROPERTY:WITH_CXX_EXCEPTIONS>>,-fexceptions,-fno-exceptions>>")

# Clang uses sjlj exceptions on i386 when targeting mingw
if(ARCH STREQUAL "i386")
    add_compile_options("$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<BOOL:$<TARGET_PROPERTY:WITH_CXX_EXCEPTIONS>>>:-fsjlj-exceptions>")
endif()

# Clang/LLVM runtime libraries
# No mingw compiler-rt builtins available, use GCC's libgcc.a for 128-bit builtins
# (__udivti3, __umodti3, etc.) that Clang emits but compiler-rt doesn't provide.
find_program(_mingw_gcc NAMES ${_gnu_tool_triplet}-gcc)
find_program(_mingw_gxx NAMES ${_gnu_tool_triplet}-g++)

function(_query_mingw_runtime _outvar _compiler)
    if(NOT _compiler)
        set(${_outvar} "" PARENT_SCOPE)
        return()
    endif()

    execute_process(
        COMMAND ${_compiler} ${ARGN}
        OUTPUT_VARIABLE _runtime_path
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)

    if(IS_ABSOLUTE "${_runtime_path}" AND EXISTS "${_runtime_path}")
        set(${_outvar} "${_runtime_path}" PARENT_SCOPE)
    else()
        set(${_outvar} "" PARENT_SCOPE)
    endif()
endfunction()

_query_mingw_runtime(_gcc_libgcc ${_mingw_gcc} -print-libgcc-file-name)
add_library(libgcc INTERFACE)
if(_gcc_libgcc)
    # Also link kernel32_vista for condition variable APIs needed by GCC's threading code
    target_link_libraries(libgcc INTERFACE ${_gcc_libgcc} libkernel32 libkernel32_vista)
else()
    target_link_libraries(libgcc INTERFACE libkernel32)
endif()

# Ensure __udivti3 and other 128-bit builtins are available for all user-mode targets.
# GCC links libgcc automatically; Clang does not when -nostdlib is used.
# Allow multiple definitions because ReactOS provides its own __chkstk_ms (also in libgcc).
if(_gcc_libgcc)
    link_libraries(${_gcc_libgcc})
    add_link_options(-Wl,--allow-multiple-definition)
endif()

# libwinpthread - not needed with pure Clang, provide as empty interface
add_library(libwinpthread INTERFACE)

# libsupc++ - use the real library from the GCC cross-compiler for C++ exception handling
# (__cxa_begin_catch, __cxa_end_catch, _Unwind_Resume, std::terminate, etc.)
_query_mingw_runtime(_gcc_libsupc++ ${_mingw_gxx} -print-file-name=libsupc++.a)
_query_mingw_runtime(_gcc_libgcc_eh ${_mingw_gcc} -print-file-name=libgcc_eh.a)
add_library(libsupc++ INTERFACE IMPORTED GLOBAL)
if(_gcc_libsupc++)
    set(_supc++_deps stdc++compat ${_gcc_libsupc++})
    if(_gcc_libgcc_eh)
        list(APPEND _supc++_deps ${_gcc_libgcc_eh})
    endif()
    # libsupc++ also needs libgcc for __gthr_win32_* threading functions
    target_link_libraries(libsupc++ INTERFACE ${_supc++_deps} libgcc)
else()
    target_link_libraries(libsupc++ INTERFACE stdc++compat)
endif()

# libmingwex provides mingw CRT functions needed by libstdc++ (fseeko64, __mingw_strtof, etc.)
_query_mingw_runtime(_mingwex_lib ${_mingw_gcc} -print-file-name=libmingwex.a)
if(_mingwex_lib)
    add_library(libmingwex STATIC IMPORTED GLOBAL)
    set_target_properties(libmingwex PROPERTIES IMPORTED_LOCATION ${_mingwex_lib})
else()
    add_library(libmingwex INTERFACE)
endif()

# Use GCC's libstdc++.a for full C++ standard library support
_query_mingw_runtime(_gcc_libstdcxx ${_mingw_gxx} -print-file-name=libstdc++.a)
add_library(libstdc++ STATIC IMPORTED GLOBAL)
if(_gcc_libstdcxx)
    set_target_properties(libstdc++ PROPERTIES IMPORTED_LOCATION ${_gcc_libstdcxx})
    target_link_libraries(libstdc++ INTERFACE libsupc++ libmingwex oldnames)
else()
    message(WARNING "libstdc++.a not found -- C++ STL targets will fail to link")
    target_link_libraries(libstdc++ INTERFACE libsupc++ oldnames)
endif()
target_compile_definitions(libstdc++ INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:PAL_STDCPP_COMPAT>")

# Create our alias libraries
add_library(cppstl ALIAS libstdc++)

else() # NOT _REACTOS_CLANG_BUILD_CONFIG

message(FATAL_ERROR "Use toolchain-clang.cmake as CMAKE_TOOLCHAIN_FILE")

endif() # _REACTOS_CLANG_BUILD_CONFIG
