
if(MSVC)
macro(MACRO_MC FILE)
    set (COMMAND_MC mc -r ${REACTOS_BINARY_DIR}/include/reactos -h ${REACTOS_BINARY_DIR}/include/reactos ${REACTOS_SOURCE_DIR}/include/reactos/mc/${FILE}.mc)
endmacro()
else()
macro(MACRO_MC FILE)
    set(COMMAND_MC ${MINGW_PREFIX}windmc -A -b ${REACTOS_SOURCE_DIR}/include/reactos/mc/${FILE}.mc -r ${REACTOS_BINARY_DIR}/include/reactos -h ${REACTOS_BINARY_DIR}/include/reactos)
endmacro()
endif()

macro (MACRO_ADD_MESSAGE_HEADERS)

  foreach(_in_FILE ${ARGN})

    get_filename_component(FILE ${_in_FILE} NAME_WE)
    MACRO_MC(${FILE})
    add_custom_command(
      OUTPUT ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.rc ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.h
      COMMAND ${COMMAND_MC}
      DEPENDS ${REACTOS_SOURCE_DIR}/include/reactos/mc/${FILE}.mc
    )
    set_source_files_properties(
      ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.h ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.rc
      PROPERTIES GENERATED TRUE
    )
    add_custom_target(${FILE} ALL DEPENDS ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.h ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.rc)

  endforeach(_in_FILE ${ARGN})

endmacro (MACRO_ADD_MESSAGE_HEADERS)

list(APPEND SOURCE
    bugcodes.mc
    errcodes.mc
    neteventmsg.mc
    ntstatus.mc
    pciclass.mc)
MACRO_ADD_MESSAGE_HEADERS(${SOURCE})
