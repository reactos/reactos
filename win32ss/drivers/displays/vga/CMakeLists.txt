
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

add_library(vgaddi MODULE
    ${SOURCE}
    vgaddi.rc)

if(SANITIZE_UB)
    target_link_libraries(vgaddi ksanitize_w32k)
endif()

set_module_type(vgaddi kerneldll ENTRYPOINT DrvEnableDriver 12)
add_pch(vgaddi vgaddi.h SOURCE)
target_link_libraries(vgaddi libcntpr)
add_importlibs(vgaddi win32k)
add_dependencies(vgaddi psdk)
add_cd_file(TARGET vgaddi DESTINATION reactos/system32 FOR all)
