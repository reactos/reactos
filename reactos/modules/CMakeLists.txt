
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/rostests/)
    add_subdirectory(rostests)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/rosapps/)
    add_subdirectory(rosapps)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/wallpapers/)
    add_subdirectory(wallpapers)
endif()

## Extra files for Boot/Live/Hybrid CD. You need to put them under [boot|live|hybrid]cd_extras
## in the same layout as you want them to be in the CD.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bootcd_extras/)
    file(GLOB_RECURSE EXTRA_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/bootcd_extras/ ${CMAKE_CURRENT_SOURCE_DIR}/bootcd_extras/*)
    foreach(item ${EXTRA_FILES})
        add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/bootcd_extras/${item} DESTINATION extras NAME_ON_CD ${item} FOR bootcd regtest)
    endforeach(item)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/livecd_extras/)
    file(GLOB_RECURSE EXTRA_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/livecd_extras/ ${CMAKE_CURRENT_SOURCE_DIR}/livecd_extras/*)
    foreach(item ${EXTRA_FILES})
        add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/livecd_extras/${item} DESTINATION extras NAME_ON_CD ${item} FOR livecd)
    endforeach(item)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/hybridcd_extras/)
    file(GLOB_RECURSE EXTRA_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/hybridcd_extras/ ${CMAKE_CURRENT_SOURCE_DIR}/hybridcd_extras/*)
    foreach(item ${EXTRA_FILES})
        add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/hybridcd_extras/${item} DESTINATION extras NAME_ON_CD ${item} FOR hybridcd)
    endforeach(item)
endif()
