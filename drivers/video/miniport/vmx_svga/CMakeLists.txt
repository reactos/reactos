
list(APPEND SOURCE vmx_svga.c vmx_svga.rc)

add_library(vmx_svga SHARED ${CMAKE_CURRENT_BINARY_DIR}/vmx_svga_precomp.h.gch ${SOURCE})

set_module_type(vmx_svga kernelmodedriver)
add_importlibs(vmx_svga ntoskrnl videoprt)

add_pch(vmx_svga ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
