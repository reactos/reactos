
add_subdirectory(dc21x4)
add_subdirectory(e100)
add_subdirectory(e1000)
add_subdirectory(ne2000)
add_subdirectory(netkvm)
if(ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64")
    # Integrated into x86 chipsets
    add_subdirectory(nvnet)
endif()
add_subdirectory(pcnet)
add_subdirectory(rtl8139)
