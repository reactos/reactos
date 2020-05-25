
set(SYSMAIN_SDB ${CMAKE_CURRENT_BINARY_DIR}/sysmain.sdb)
set(DRVMAIN_SDB ${CMAKE_CURRENT_BINARY_DIR}/drvmain.sdb)

add_custom_command(OUTPUT ${SYSMAIN_SDB}
                   COMMAND native-xml2sdb -i ${CMAKE_CURRENT_SOURCE_DIR}/sysmain.xml -o ${SYSMAIN_SDB}
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sysmain.xml native-xml2sdb)

add_custom_command(OUTPUT ${DRVMAIN_SDB}
                   COMMAND native-xml2sdb -i ${CMAKE_CURRENT_SOURCE_DIR}/drvmain.xml -o ${DRVMAIN_SDB}
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/drvmain.xml native-xml2sdb)

add_custom_target(compatdb DEPENDS
    ${SYSMAIN_SDB}
    ${DRVMAIN_SDB}
)
add_cd_file(TARGET compatdb FILE ${SYSMAIN_SDB} ${DRVMAIN_SDB} DESTINATION reactos/AppPatch FOR all)
