
list(APPEND SOURCE vmx_svga.c vmx_svga.rc)

add_library(vmx_svga SHARED ${CMAKE_CURRENT_BINARY_DIR}/vmx_svga_precomp.h.gch ${SOURCE})

set_target_properties(vmx_svga PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native -Wl,--exclude-all-symbols" SUFFIX ".sys")

add_importlibs(vmx_svga ntoskrnl videoprt)
add_pch(vmx_svga ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(vmx_svga psdk bugcodes)
