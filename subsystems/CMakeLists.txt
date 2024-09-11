
add_subdirectory(csr)
if(ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64") #TODO: Cross compile support for ARM32
add_subdirectory(mvdm)
endif()
add_subdirectory(win)
