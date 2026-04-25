## EXPERIMENTAL!!

# We need to use almost the same tricks as the ones used for MSVC 'add_asm_files'
# support because we are going to compile ASM files for a fixed target (16-bit x86)
# that is different from the main target.

if(NOT MSVC)
###
### For GCC / Clang
###

# Clang's LLVM integrated assembler does not emit 16-bit ELF relocations for
# the i386-unknown-elf target (errors out on `mov bx, offset Sym`, `.word Sym`,
# etc.), so route 16-bit MASM-style assembly through the i686 GNU `as` from
# the RosBE GCC toolchain.
if(CMAKE_C_COMPILER_ID STREQUAL "Clang" AND NOT ASM16_GAS)
    set(_asm16_gas_hints "")
    if(REACTOS_CLANG_GCC_TOOLCHAIN)
        get_filename_component(_rosbe_mingw_root "${REACTOS_CLANG_GCC_TOOLCHAIN}" DIRECTORY)
        list(APPEND _asm16_gas_hints
            "${_rosbe_mingw_root}/i686-w64-mingw32/bin"
            "${REACTOS_CLANG_GCC_TOOLCHAIN}/bin")
    endif()
    find_program(ASM16_GAS NAMES i686-w64-mingw32-as
        HINTS ${_asm16_gas_hints})
    if(NOT ASM16_GAS)
        message(FATAL_ERROR
            "asm16.cmake: cannot find i686-w64-mingw32-as for 16-bit MASM-style "
            "assembly. Pass -DREACTOS_CLANG_GCC_TOOLCHAIN=<rosbe mingw-gcc dir> "
            "or place i686-w64-mingw32-as on PATH.")
    endif()
endif()

function(add_asm16_bin _target _binary_file _base_address)
    set(_concatenated_asm_file ${CMAKE_CURRENT_BINARY_DIR}/${_target}.asm)
    set(_preprocessed_file ${CMAKE_CURRENT_BINARY_DIR}/${_target}.pp.s)
    set(_object_file ${CMAKE_CURRENT_BINARY_DIR}/${_target}.o)

    # unset(_source_file_list)

    get_defines(_directory_defines)
    get_includes(_directory_includes)
    get_directory_property(_defines COMPILE_DEFINITIONS)

    # Build a list of all the defines needed.
    foreach(_source_file ${ARGN})
        get_filename_component(_source_file_full_path ${_source_file} ABSOLUTE)
        get_source_file_property(_defines_semicolon_list ${_source_file_full_path} COMPILE_DEFINITIONS)

        # unset(_source_file_defines)

        foreach(_define ${_defines_semicolon_list})
            if(NOT ${_define} STREQUAL "NOTFOUND")
                list(APPEND _source_file_defines -D${_define})
            endif()
        endforeach()

        list(APPEND _source_file_list ${_source_file_full_path})
    endforeach()

    # We do not support 16-bit ASM linking so the only way to compile
    # many ASM files is by concatenating them into a single one and
    # compile the resulting file.
    concatenate_files(${_concatenated_asm_file} ${_source_file_list})
    set_source_files_properties(${_concatenated_asm_file} PROPERTIES GENERATED TRUE)

    if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
        # Step 1: CPP preprocess via Clang. We feed the result to GNU `as`,
        # not Clang's integrated assembler, so undefine __clang__ to keep
        # asm.inc on the GAS path (otherwise it adds case-only aliases that
        # collide under MinGW GAS's case-insensitive macro names).
        add_custom_command(
            OUTPUT ${_preprocessed_file}
            COMMAND ${CMAKE_ASM_COMPILER} -E -x assembler-with-cpp -U__clang__ -o ${_preprocessed_file} -I${REACTOS_SOURCE_DIR}/sdk/include/asm -I${REACTOS_BINARY_DIR}/sdk/include/asm ${_directory_includes} ${_source_file_defines} ${_directory_defines} -D__ASM__ ${_concatenated_asm_file}
            DEPENDS ${_concatenated_asm_file})

        # Step 2: Assemble with GNU as (handles 16-bit relocations).
        add_custom_command(
            OUTPUT ${_object_file}
            COMMAND ${ASM16_GAS} ${_preprocessed_file} -o ${_object_file}
            DEPENDS ${_preprocessed_file})

        # Step 3: Flatten to a raw binary at the expected real-mode load address
        add_custom_command(
            OUTPUT ${_binary_file}
            COMMAND native-obj2bin ${_object_file} ${_binary_file} ${_base_address}
            DEPENDS ${_object_file} native-obj2bin)
    else()
        ##
        ## All this part is the same as CreateBootSectorTarget
        ##
        add_custom_command(
            OUTPUT ${_object_file}
            COMMAND ${CMAKE_ASM_COMPILER} -x assembler-with-cpp -o ${_object_file} -I${REACTOS_SOURCE_DIR}/sdk/include/asm -I${REACTOS_BINARY_DIR}/sdk/include/asm ${_directory_includes} ${_source_file_defines} ${_directory_defines} -D__ASM__ -c ${_concatenated_asm_file}
            DEPENDS ${_concatenated_asm_file})

        add_custom_command(
            OUTPUT ${_binary_file}
            COMMAND native-obj2bin ${_object_file} ${_binary_file} ${_base_address}
            # COMMAND objcopy --output-target binary --image-base 0x${_base_address} ${_object_file} ${_binary_file}
            DEPENDS ${_object_file} native-obj2bin)
    endif()

    add_custom_target(${_target} ALL DEPENDS ${_binary_file})
    # set_target_properties(${_target} PROPERTIES OUTPUT_NAME ${_target} SUFFIX ".bin")
    set_target_properties(${_target} PROPERTIES BINARY_PATH ${_binary_file})
    add_clean_target(${_target})
endfunction()

else()
###
### For MSVC
###
function(add_asm16_bin _target _binary_file _base_address)
    set(_concatenated_asm_file ${CMAKE_CURRENT_BINARY_DIR}/${_target}.asm)
    set(_preprocessed_asm_file ${CMAKE_CURRENT_BINARY_DIR}/${_target}.tmp)
    set(_object_file ${CMAKE_CURRENT_BINARY_DIR}/${_target}.obj)

    # unset(_source_file_list)

    get_defines(_directory_defines)
    get_includes(_directory_includes)
    get_directory_property(_defines COMPILE_DEFINITIONS)

    # Build a list of all the defines needed.
    foreach(_source_file ${ARGN})
        get_filename_component(_source_file_full_path ${_source_file} ABSOLUTE)
        get_source_file_property(_defines_semicolon_list ${_source_file_full_path} COMPILE_DEFINITIONS)

        # unset(_source_file_defines)

        foreach(_define ${_defines_semicolon_list})
            if(NOT ${_define} STREQUAL "NOTFOUND")
                list(APPEND _source_file_defines -D${_define})
            endif()
        endforeach()

        list(APPEND _source_file_list ${_source_file_full_path})
    endforeach()

    # We do not support 16-bit ASM linking so the only way to compile
    # many ASM files is by concatenating them into a single one and
    # compile the resulting file.
    concatenate_files(${_concatenated_asm_file} ${_source_file_list})
    set_source_files_properties(${_concatenated_asm_file} PROPERTIES GENERATED TRUE)

    ##
    ## All this part is the same as CreateBootSectorTarget
    ##
    if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
        set(_no_std_includes_flag "-nostdinc")
    else()
        set(_no_std_includes_flag "/X")
    endif()

    add_custom_command(
        OUTPUT ${_preprocessed_asm_file}
        #COMMAND ${CMAKE_C_COMPILER} /nologo ${_no_std_includes_flag} /I${REACTOS_SOURCE_DIR}/sdk/include/asm /I${REACTOS_BINARY_DIR}/sdk/include/asm ${_directory_includes} ${_source_file_defines} ${_directory_defines} /D__ASM__ /D_USE_ML /EP /c ${_concatenated_asm_file} > ${_preprocessed_asm_file}
        COMMAND cl /nologo /X /I${REACTOS_SOURCE_DIR}/sdk/include/asm /I${REACTOS_BINARY_DIR}/sdk/include/asm ${_directory_includes} ${_source_file_defines} ${_directory_defines} /D__ASM__ /D_USE_ML /EP /c ${_concatenated_asm_file} > ${_preprocessed_asm_file}
        DEPENDS ${_concatenated_asm_file})

    if(MSVC_VERSION GREATER_EQUAL 1936)
        set(_quiet_flag "/quiet")
    endif()
    set(_pp_asm16_compile_command ${CMAKE_ASM16_COMPILER} /nologo ${_quiet_flag} /Cp /Fo${_object_file} /c /Ta ${_preprocessed_asm_file})

    add_custom_command(
        OUTPUT ${_object_file}
        COMMAND ${_pp_asm16_compile_command}
        DEPENDS ${_preprocessed_asm_file})

    add_custom_command(
        OUTPUT ${_binary_file}
        COMMAND native-obj2bin ${_object_file} ${_binary_file} ${_base_address}
        DEPENDS ${_object_file} native-obj2bin)

    add_custom_target(${_target} ALL DEPENDS ${_binary_file})
    # set_target_properties(${_target} PROPERTIES OUTPUT_NAME ${_target} SUFFIX ".bin")
    set_target_properties(${_target} PROPERTIES BINARY_PATH ${_binary_file})
    add_clean_target(${_target})
endfunction()

endif()
