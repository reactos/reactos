
list(APPEND SOURCE vmx_svga.c vmx_svga.rc)

add_library(vmx_svga SHARED ${SOURCE})

set_module_type(vmx_svga kernelmodedriver)
add_importlibs(vmx_svga ntoskrnl videoprt)
