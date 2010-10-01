
remove_definitions(-DWINVER=0x502)
add_definitions(-DWINVER=0x600)

include_directories(.)

get_directory_property(defines COMPILE_DEFINITIONS)
get_directory_property(includes INCLUDE_DIRECTORIES)

foreach(arg ${defines})
  set(result_defs ${result_defs} -D${arg})
endforeach(arg ${defines})

foreach(arg ${includes})
  set(result_incs -I${arg} ${result_incs})
endforeach(arg ${includes})

MACRO (MACRO_IDL_FILES)

  FOREACH(_in_FILE ${ARGN})

    get_filename_component(FILE ${_in_FILE} NAME_WE)
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_s.h ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_s.c
      COMMAND native-widl ${result_incs} ${result_defs} -m32 --win32 -h -H ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_s.h -s -S ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_s.c ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.idl
      DEPENDS native-widl
    )
    set_source_files_properties(
      ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_s.h ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_s.c
      PROPERTIES GENERATED TRUE
    )
    add_library(${FILE}_server ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_s.c)
    
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_c.h ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_c.c
      COMMAND native-widl ${result_incs} ${result_defs} -m32 --win32 -h -H ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_c.h -c -C ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_c.c ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.idl
      DEPENDS native-widl
    )
    set_source_files_properties(
      ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_c.h ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_c.c
      PROPERTIES GENERATED TRUE
    )
    add_library(${FILE}_client ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_c.c)

  ENDFOREACH(_in_FILE ${ARGN})

ENDMACRO (MACRO_IDL_FILES)

file(GLOB_RECURSE SOURCE "*.idl")
LIST(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/ms-dtyp.idl)
MACRO_IDL_FILES(${SOURCE})
