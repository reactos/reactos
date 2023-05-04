
add_subdirectory(bootsect)
endif()
add_subdirectory(fdebug)
add_subdirectory(freeldr)
if(ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64")
add_subdirectory(install)
