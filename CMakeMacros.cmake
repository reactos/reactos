MACRO(_PCH_GET_COMPILE_FLAGS _target_name _out_compile_flags _header_filename)

    # Add the precompiled header to the build
    SET(_gch_filename "${_header_filename}.gch")
    LIST(APPEND ${_out_compile_flags} -c ${_header_filename} -o ${_gch_filename})

    # This gets us our includes
    GET_DIRECTORY_PROPERTY(DIRINC INCLUDE_DIRECTORIES)
    FOREACH(item ${DIRINC})
        LIST(APPEND ${_out_compile_flags} -I${item})
    ENDFOREACH(item) 

    # This is a particular bit of undocumented/hacky magic I'm quite proud of
    GET_DIRECTORY_PROPERTY(_compiler_flags DEFINITIONS)
    STRING(REPLACE "\ " "\t" _compiler_flags ${_compiler_flags})
    LIST(APPEND ${_out_compile_flags} ${_compiler_flags})

    # This gets any specific definitions that were added with set-target-property
    GET_TARGET_PROPERTY(_target_defs ${_target_name} COMPILE_DEFINITIONS)
    IF (_target_defs)
    	FOREACH(item ${_target_defs})
            LIST(APPEND ${_out_compile_flags} -D${item})
   	ENDFOREACH(item)
    ENDIF()

ENDMACRO(_PCH_GET_COMPILE_FLAGS) 

MACRO(add_pch _target_name _header_filename _src_list)

	SET(_gch_filename "${_header_filename}.gch")

	LIST(APPEND ${_src_list} ${_gch_filename})

	_PCH_GET_COMPILE_FLAGS(${_target_name} _args ${_header_filename})

	add_custom_command(OUTPUT ${_gch_filename}
		   COMMAND rm -f ${_gch_filename}
		   COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_COMPILER_ARG1} ${_args}
			    DEPENDS ${_header_filename})
ENDMACRO(add_pch _target_name _header_filename _src_list)
