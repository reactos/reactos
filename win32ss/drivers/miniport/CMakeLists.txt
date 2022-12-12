
add_subdirectory(bochs)
if(ARCH STREQUAL "i386" OR ARCH STREQUAL "amd64")
    add_subdirectory(vbe)
    add_subdirectory(vga)
endif()
#add_subdirectory(vga_new)
add_subdirectory(vmx_svga)

if(ARCH STREQUAL "i386")
    add_subdirectory(pc98vid)
    add_subdirectory(xboxvmp)
endif()
