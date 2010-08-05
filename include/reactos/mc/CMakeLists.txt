MACRO (MACRO_ADD_MESSAGE_HEADERS)

  FOREACH(_in_FILE ${ARGN})

    get_filename_component(FILE ${_in_FILE} NAME_WE)
    add_custom_command(
      OUTPUT ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.rc ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.h
      COMMAND ${MINGW_PREFIX}windmc -A ${REACTOS_SOURCE_DIR}/include/reactos/mc/${FILE}.mc -r ${REACTOS_BINARY_DIR}/include/reactos -h ${REACTOS_BINARY_DIR}/include/reactos
    )
    set_source_files_properties(
      ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.h ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.rc
      PROPERTIES GENERATED TRUE
    )
    add_custom_target(${FILE} ALL DEPENDS ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.h ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.rc)

  ENDFOREACH(_in_FILE ${ARGN})

ENDMACRO (MACRO_ADD_MESSAGE_HEADERS)

file(GLOB_RECURSE SOURCE "*.mc")
MACRO_ADD_MESSAGE_HEADERS(${SOURCE})