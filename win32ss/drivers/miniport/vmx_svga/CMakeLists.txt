
add_library(vmx_svga MODULE vmx_svga.c vmx_svga.rc)
set_module_type(vmx_svga kernelmodedriver)
add_importlibs(vmx_svga ntoskrnl videoprt)
add_cd_file(TARGET vmx_svga DESTINATION reactos/system32/drivers FOR all)
add_registry_inf(vmx_svga_reg.inf)
add_driver_inf(vmx_svga vmx_svga.inf)
