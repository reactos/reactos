
spec2def(vgaddi.dll vgaddi.spec)

list(APPEND SOURCE
    main/enable.c
    objects/screen.c
    objects/pointer.c
    objects/lineto.c
    objects/paint.c
    objects/bitblt.c
    objects/transblt.c
    objects/offscreen.c
    objects/copybits.c
    vgavideo/vgavideo.c
    vgaddi.h)

add_library(vgaddi SHARED
    ${SOURCE}
    vgaddi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/vgaddi.def)

set_entrypoint(vgaddi DrvEnableDriver 12)
set_subsystem(vgaddi native)
set_image_base(vgaddi 0x00010000)
add_pch(vgaddi vgaddi.h SOURCE)
add_importlibs(vgaddi win32k)
target_link_libraries(vgaddi libcntpr)
add_dependencies(vgaddi psdk bugcodes)
add_cd_file(TARGET vgaddi DESTINATION reactos/system32 FOR all)
