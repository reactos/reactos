
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/sysmain.sdb
                   COMMAND native-xml2sdb -i ${CMAKE_CURRENT_SOURCE_DIR}/sysmain.xml -o ${CMAKE_CURRENT_BINARY_DIR}/sysmain.sdb
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sysmain.xml native-xml2sdb)

add_custom_target(compatdb DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/sysmain.sdb)
add_cd_file(TARGET compatdb FILE ${CMAKE_CURRENT_BINARY_DIR}/sysmain.sdb DESTINATION reactos/AppPatch FOR all)
