
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
