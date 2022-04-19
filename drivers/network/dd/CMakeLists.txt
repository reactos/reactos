
add_subdirectory(e1000)
add_subdirectory(ne2000)
add_subdirectory(netkvm)
if(ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64")
    add_subdirectory(nvnet)
endif()
add_subdirectory(pcnet)
add_subdirectory(rtl8139)
