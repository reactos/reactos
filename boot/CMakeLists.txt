
include(boot_images.cmake)

add_subdirectory(bootdata)
# add_subdirectory(environ)
add_subdirectory(freeldr)
add_subdirectory(rtl)

add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/remaster.cmd DESTINATION loader NO_CAB NOT_IN_HYBRIDCD FOR bootcd livecd hybridcd)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/remaster.sh DESTINATION loader NO_CAB NOT_IN_HYBRIDCD FOR bootcd livecd hybridcd)
