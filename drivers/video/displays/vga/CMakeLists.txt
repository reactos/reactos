
spec2def(vgaddi ${CMAKE_CURRENT_SOURCE_DIR}/vgaddi.spec ${CMAKE_CURRENT_BINARY_DIR}/vgaddi.def)

add_library(vgaddi SHARED
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
    vgaddi.rc)

set_target_properties(vgaddi PROPERTIES LINK_FLAGS "-Wl,-entry,_DrvEnableDriver@12 -Wl,--subsystem,native -Wl,--image-base,0x00010000")

target_link_libraries(vgaddi ${CMAKE_CURRENT_BINARY_DIR}/vgaddi.def)

add_importlibs(vgaddi win32k)
add_dependencies(vgaddi vgaddi_def psdk bugcodes)
