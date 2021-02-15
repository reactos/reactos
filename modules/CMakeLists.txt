
if(ENABLE_ROSAPPS)
    add_subdirectory(rosapps)
endif()

if(ENABLE_ROSTESTS)
    add_subdirectory(rostests)
endif()

if(ENABLE_WALLPAPERS)
    add_subdirectory(wallpapers)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/AHK_Tests/)
    add_subdirectory(AHK_Tests)
endif()

## Extra files for Boot/Live/Hybrid CD. You need to put them under [boot|live|hybrid]cd_extras
## in the same layout as you want them to be in the CD.
## If you change the content of any of the following folders later on,
## run "cmake ." before compiling.
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
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/hybridcd_extras/)
    file(GLOB_RECURSE EXTRA_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/hybridcd_extras/ ${CMAKE_CURRENT_SOURCE_DIR}/hybridcd_extras/*)
    foreach(item ${EXTRA_FILES})
        add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/hybridcd_extras/${item} DESTINATION extras NO_CAB NAME_ON_CD ${item} FOR hybridcd)
    endforeach(item)
endif()

## Extra files which you need existing on the hard drive after the installation.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/)
    file(GLOB_RECURSE EXTRA_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/*)
    foreach(item ${EXTRA_FILES})
        add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/${item} DESTINATION reactos/3rdParty NAME_ON_CD ${item} FOR bootcd)
    endforeach(item)
endif()
