
if(NOT ${ARCH} MATCHES amd64)
    add_subdirectory(nt4compat)
endif()
add_subdirectory(usbd)
#add_subdirectory(usbehci) The USB branch has usbehci_new
#add_subdirectory(usbhub) Compiles, just skipped in trunk
#add_subdirectory(usbstor) Compiles, just skipped in trunk
