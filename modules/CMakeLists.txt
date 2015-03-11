
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/rostests/)
    add_subdirectory(rostests)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/rosapps/)
    add_subdirectory(rosapps)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/wallpaper/)
    add_subdirectory(wallpaper)
endif()

## Extra files for Hybrid CD. You need to put them under hybridcd_extras
## in the same layout as you want them to be in the CD.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/hybridcd_extras/)
    file(GLOB_RECURSE EXTRA_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/hybridcd_extras/ ${CMAKE_CURRENT_SOURCE_DIR}/hybridcd_extras/*)
    foreach(item ${EXTRA_FILES})
        add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/hybridcd_extras/${item} DESTINATION extras NAME_ON_CD ${item} FOR hybridcd)
    endforeach(item)
endif()
