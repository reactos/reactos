
if(SARCH STREQUAL "pc98")
    # Actual binary filename is vga.sys
    add_subdirectory(pc98vid)
else()
    add_subdirectory(vbe)
    add_subdirectory(vga)
    add_subdirectory(vga_new)
    add_subdirectory(vmx_svga)

    if(ARCH STREQUAL "i386")
        add_subdirectory(xboxvmp)
    endif()
endif()
