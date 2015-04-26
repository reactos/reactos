
start_module_group(rostests)

add_subdirectory(apitests)
#add_subdirectory(dibtests)
add_subdirectory(drivers)
#add_subdirectory(dxtest)
add_subdirectory(kmtests)
#add_subdirectory(regtests)
add_subdirectory(rosautotest)
add_subdirectory(tests)
#add_subdirectory(win32)
add_subdirectory(winetests)


## Extra files needed for the various tests can be added into the "testdata" subdirectory.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/testdata/)
    file(GLOB_RECURSE TESTDATA_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/testdata/ ${CMAKE_CURRENT_SOURCE_DIR}/testdata/*)
    foreach(item ${TESTDATA_FILES})
        add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/testdata/${item} DESTINATION reactos/bin/testdata NAME_ON_CD ${item} FOR regtest)
    endforeach(item)
endif()

end_module_group()
