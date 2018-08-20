add_subdirectory(applications)
add_subdirectory(demos)
#add_subdirectory(drivers)
add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(templates)

list(APPEND ROSAPPS_INF_FILES rosapps_shortcuts.inf)

foreach(item ${ROSAPPS_INF_FILES})
    set(_rosapps_converted_item ${CMAKE_CURRENT_BINARY_DIR}/${item})
    set(_rosapps_source_item ${CMAKE_CURRENT_SOURCE_DIR}/${item})
    add_custom_command(OUTPUT "${_rosapps_converted_item}"
                       COMMAND native-utf16le "${_rosapps_source_item}" "${_rosapps_converted_item}"
                       DEPENDS native-utf16le "${_rosapps_source_item}")
    add_cd_file(TARGET converted_inf_files2 FILE ${_rosapps_converted_item} DESTINATION reactos/inf FOR all)
    list(APPEND rosapps_converted_item ${_rosapps_converted_item})
endforeach(item)
add_custom_target(converted_inf_files2 DEPENDS ${rosapps_converted_item})
