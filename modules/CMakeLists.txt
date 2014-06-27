
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/rostests/)
add_subdirectory(rostests)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/rosapps/)
add_subdirectory(rosapps)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/wallpaper/)
add_subdirectory(wallpaper)
endif()
