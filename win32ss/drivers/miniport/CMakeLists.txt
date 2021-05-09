
if(SARCH STREQUAL "pc98")
    # Actual binary filename is vga.sys
    add_subdirectory(pc98vid)
elseif(SARCH STREQUAL "xbox")
    add_subdirectory(xboxvmp)
else()
    add_subdirectory(vbe)
    add_subdirectory(vga)
    add_subdirectory(vga_new)
    add_subdirectory(vmx_svga)
endif()
