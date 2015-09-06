
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
    vgaddi.rc)

set_module_type(vgaddi kerneldll ENTRYPOINT DrvEnableDriver 12)
add_pch(vgaddi vgaddi.h SOURCE)
add_importlibs(vgaddi win32k)
target_link_libraries(vgaddi libcntpr)
add_dependencies(vgaddi psdk)
add_cd_file(TARGET vgaddi DESTINATION reactos/system32 FOR all)
