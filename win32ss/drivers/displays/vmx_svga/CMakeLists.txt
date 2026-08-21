list(APPEND SOURCE
    enable.c
    palette.c
    pointer.c
    render.c
    screen.c
    surface.c
    vmx_svga_disp.h)

add_library(vmx_svga_disp MODULE
    ${SOURCE}
    vmx_svga_disp.rc)

set_module_type(vmx_svga_disp kerneldll ENTRYPOINT DrvEnableDriver 12)
add_pch(vmx_svga_disp vmx_svga_disp.h SOURCE)
target_link_libraries(vmx_svga_disp libcntpr)
add_importlibs(vmx_svga_disp win32k)
add_dependencies(vmx_svga_disp psdk)
add_cd_file(TARGET vmx_svga_disp DESTINATION reactos/system32 FOR all)
