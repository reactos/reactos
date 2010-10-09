
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

MACRO(spec2def _target_name _spec_file _def_file)

    add_custom_command(
        OUTPUT ${_def_file}
        COMMAND native-winebuild -o ${_def_file} --def -E ${_spec_file} --filename ${_target_name}.dll
        DEPENDS native-winebuild)
    set_source_files_properties(${_def_file} PROPERTIES GENERATED TRUE)
    add_custom_target(${_target_name}_def ALL DEPENDS ${_def_file})

ENDMACRO(spec2def _target_name _spec_file _def_file)

if (NOT MSVC)
MACRO(CreateBootSectorTarget _target_name _asm_file _object_file)

    get_filename_component(OBJECT_PATH ${_object_file} PATH)
    get_filename_component(OBJECT_NAME ${_object_file} NAME)
    file(MAKE_DIRECTORY ${OBJECT_PATH})
    get_directory_property(defines COMPILE_DEFINITIONS)
    get_directory_property(includes INCLUDE_DIRECTORIES)

    foreach(arg ${defines})
        set(result_defs ${result_defs} -D${arg})
    endforeach(arg ${defines})

    foreach(arg ${includes})
        set(result_incs -I${arg} ${result_incs})
    endforeach(arg ${includes})

    add_custom_command(
        OUTPUT ${_object_file}
        COMMAND nasm -o ${_object_file} ${result_incs} ${result_defs} -f bin ${_asm_file}
        DEPENDS native-winebuild)
    set_source_files_properties(${_object_file} PROPERTIES GENERATED TRUE)
    add_custom_target(${_target_name} ALL DEPENDS ${_object_file})
    add_minicd(${_object_file} loader ${OBJECT_NAME})
ENDMACRO(CreateBootSectorTarget _target_name _asm_file _object_file)
else()
MACRO(CreateBootSectorTarget _target_name _asm_file _object_file)
ENDMACRO()
endif()

MACRO(MACRO_IDL_COMPILE_OBJECT OBJECT SOURCE)
  GET_PROPERTY(FLAGS SOURCE ${SOURCE} PROPERTY COMPILE_FLAGS)
  GET_PROPERTY(DEFINES SOURCE ${SOURCE} PROPERTY COMPILE_DEFINITIONS)
  GET_PROPERTY(INCLUDE_DIRECTORIES DIRECTORY PROPERTY INCLUDE_DIRECTORIES)
  FOREACH(DIR ${INCLUDE_DIRECTORIES})
    SET(FLAGS "${FLAGS} -I${DIR}")
  ENDFOREACH()

  SET(IDL_COMMAND ${CMAKE_IDL_COMPILE_OBJECT})
  STRING(REPLACE "<CMAKE_IDL_COMPILER>" "${CMAKE_IDL_COMPILER}" IDL_COMMAND "${IDL_COMMAND}")
  STRING(REPLACE <FLAGS> "${FLAGS}" IDL_COMMAND "${IDL_COMMAND}")
  STRING(REPLACE "<DEFINES>" "${DEFINES}" IDL_COMMAND "${IDL_COMMAND}")
  STRING(REPLACE "<OBJECT>" "${OBJECT}" IDL_COMMAND "${IDL_COMMAND}")
  STRING(REPLACE "<SOURCE>" "${SOURCE}" IDL_COMMAND "${IDL_COMMAND}")
  SEPARATE_ARGUMENTS(IDL_COMMAND)

  ADD_CUSTOM_COMMAND(
    OUTPUT ${OBJECT}
    COMMAND ${IDL_COMMAND}
    DEPENDS ${SOURCE}
    VERBATIM
  )
ENDMACRO()

MACRO(ADD_INTERFACE_DEFINITIONS TARGET)
  FOREACH(SOURCE ${ARGN})
    GET_FILENAME_COMPONENT(FILE ${SOURCE} NAME_WE)
    SET(OBJECT ${CMAKE_CURRENT_BINARY_DIR}/${FILE}.h)
    MACRO_IDL_COMPILE_OBJECT(${OBJECT} ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE})
    LIST(APPEND OBJECTS ${OBJECT})
  ENDFOREACH()
  ADD_CUSTOM_TARGET(${TARGET} ALL DEPENDS ${OBJECTS})
ENDMACRO()

MACRO(add_minicd_target _targetname _dir _nameoncd)
    get_target_property(FILENAME ${_targetname} LOCATION)

    add_custom_command(
        OUTPUT ${REACTOS_BINARY_DIR}/boot/bootcd/${_dir}/${_nameoncd}        
        COMMAND ${CMAKE_COMMAND} -E copy ${FILENAME} ${BOOTCD_DIR}/${_dir}/${_nameoncd})
        
    add_custom_target(${_targetname}_minicd DEPENDS ${BOOTCD_DIR}/${_dir}/${_nameoncd})

    add_dependencies(${_targetname}_minicd ${_targetname})
    add_dependencies(minicd ${_targetname}_minicd)
ENDMACRO(add_minicd_target _targetname _dir _nameoncd)

MACRO(add_minicd FILENAME _dir _nameoncd)
    add_custom_command(
        OUTPUT ${BOOTCD_DIR}/${_dir}/${_nameoncd}
        DEPENDS ${FILENAME}
        COMMAND ${CMAKE_COMMAND} -E copy ${FILENAME} ${BOOTCD_DIR}/${_dir}/${_nameoncd})
        
    add_custom_target(${_nameoncd}_minicd DEPENDS ${BOOTCD_DIR}/${_dir}/${_nameoncd})
    
    add_dependencies(minicd ${_nameoncd}_minicd)
ENDMACRO(add_minicd)
