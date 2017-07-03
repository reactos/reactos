
if(ENABLE_ROSTESTS)
    add_subdirectory(rostests)
endif()

if(ENABLE_ROSAPPS)
    add_subdirectory(rosapps)
endif()

if(ENABLE_WALLPAPERS)
    add_subdirectory(wallpapers)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/AHK_Tests/)
    add_subdirectory(AHK_Tests)
endif()

## Extra files for the Boot/Live CD. You need to put them under "[boot|live]cd_extras"
## using the same layout as you want them to be in the CD.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bootcd_extras/)
    file(GLOB_RECURSE EXTRA_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/bootcd_extras/ ${CMAKE_CURRENT_SOURCE_DIR}/bootcd_extras/*)
    foreach(item ${EXTRA_FILES})
        add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/bootcd_extras/${item} DESTINATION extras NO_CAB NAME_ON_CD ${item} FOR bootcd regtest)
    endforeach(item)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/livecd_extras/)
    file(GLOB_RECURSE EXTRA_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/livecd_extras/ ${CMAKE_CURRENT_SOURCE_DIR}/livecd_extras/*)
    foreach(item ${EXTRA_FILES})
        add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/livecd_extras/${item} DESTINATION extras NO_CAB NAME_ON_CD ${item} FOR livecd)
    endforeach(item)
endif()
