
list(APPEND TEXTFILES
    textfiles/ExtraLargeNormal.INI
    textfiles/LargeFontsNormal.INI
    textfiles/NormalNormal.INI)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/textfiles)
foreach(_file ${TEXTFILES})
    get_filename_component(_file_name ${_file} NAME_WE)
    string(REPLACE ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} _converted_file "${_file}")
    string(REPLACE ${_file_name} "${_file_name}_utf16" _converted_file ${_converted_file})
    set(_file ${CMAKE_CURRENT_SOURCE_DIR}/${_file})
    set(_converted_file ${CMAKE_CURRENT_BINARY_DIR}/${_converted_file})
    add_custom_command(OUTPUT ${_converted_file}
                       COMMAND native-utf16le ${_file} ${_converted_file}
                       DEPENDS native-utf16le ${_file})
    list(APPEND _converted_files ${_converted_file})
endforeach()

set_source_files_properties(lautus.rc PROPERTIES OBJECT_DEPENDS "${_converted_files}")
add_library(lautus.msstyles SHARED lautus.rc)
set_module_type(lautus.msstyles module)
set_target_properties(lautus.msstyles PROPERTIES SUFFIX "")
add_cd_file(TARGET lautus.msstyles DESTINATION reactos/Resources/Themes/Lautus FOR all)
