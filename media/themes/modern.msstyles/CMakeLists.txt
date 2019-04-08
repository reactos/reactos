file(GLOB_RECURSE modern_rc_deps ${CMAKE_CURRENT_BINARY_DIR}/bitmaps/* ${CMAKE_CURRENT_BINARY_DIR}/lang/* ${CMAKE_CURRENT_BINARY_DIR}/textfiles/*)
add_rc_deps(modern.rc ${modern_rc_deps})

list(APPEND TEXTFILES
    textfiles/Light/ExtraLargeLight.INI
    textfiles/Light/LargeFontsLight.INI
    textfiles/Light/RegularLight.INI
    textfiles/Dark/ExtraLargeDark.INI
    textfiles/Dark/LargeFontsDark.INI
    textfiles/Dark/RegularDark.INI)

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

set_source_files_properties(modern.rc PROPERTIES OBJECT_DEPENDS "${_converted_files}")
add_library(modern.msstyles MODULE modern.rc)
set_module_type(modern.msstyles module)
set_target_properties(modern.msstyles PROPERTIES SUFFIX "")
add_cd_file(TARGET modern.msstyles DESTINATION reactos/Resources/Themes/Modern FOR all)
