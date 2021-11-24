
add_subdirectory(vbe)
add_subdirectory(vga)
#add_subdirectory(vga_new)
add_subdirectory(vmx_svga)

if(ARCH STREQUAL "i386")
    add_subdirectory(pc98vid)
    add_subdirectory(xboxvmp)
endif()
