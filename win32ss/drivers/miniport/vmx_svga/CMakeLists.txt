
add_library(vmx_svga MODULE vmx_svga.c vmx_svga.rc)
set_module_type(vmx_svga kernelmodedriver)
add_importlibs(vmx_svga ntoskrnl videoprt)
