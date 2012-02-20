
spec2def(win32k.sys win32k.spec)

include_directories(
    .
    include
    ${REACTOS_SOURCE_DIR}/ntoskrnl/include
    ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/include
    ${REACTOS_SOURCE_DIR}/include/reactos/subsys
    ${REACTOS_SOURCE_DIR}/include/reactos/drivers)

add_definitions(
    -DLANGPACK
    -D_WIN32K_)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dib)

list(APPEND GENDIB_FILES
    ${CMAKE_CURRENT_BINARY_DIR}/dib/dib8gen.c
    ${CMAKE_CURRENT_BINARY_DIR}/dib/dib16gen.c
    ${CMAKE_CURRENT_BINARY_DIR}/dib/dib32gen.c)

add_custom_command(
    OUTPUT ${GENDIB_FILES}
    COMMAND native-gendib ${CMAKE_CURRENT_BINARY_DIR}/dib
    DEPENDS native-gendib)

list(APPEND SOURCE
    dib/alphablend.c
    dib/dib1bpp.c
    dib/dib4bpp.c
    dib/dib8bpp.c
    dib/dib16bpp.c
    dib/dib24bpp.c
    dib/dib32bpp.c
    dib/dib.c
    dib/floodfill.c
    dib/stretchblt.c
    eng/alphablend.c
    eng/bitblt.c
    eng/engbrush.c
    eng/engevent.c
    eng/clip.c
    eng/copybits.c
    eng/debug.c
    eng/device.c
    eng/driverobj.c
    eng/error.c
    eng/float.c
    eng/gradient.c
    eng/lineto.c
    eng/ldevobj.c
    eng/mapping.c
    eng/mem.c
    eng/engmisc.c
    eng/mouse.c
    eng/paint.c
    eng/pdevobj.c
    eng/perfcnt.c
    eng/rlecomp.c
    eng/semaphor.c
    eng/sort.c
    eng/string.c
    eng/stretchblt.c
    eng/surface.c
    eng/transblt.c
    eng/engwindow.c
    eng/xlate.c
    main/dllmain.c
    misc/driver.c
    misc/file.c
    misc/math.c
    misc/rtlstr.c
    misc/copy.c
    misc/registry.c
    misc/usrheap.c
    ntddraw/ddraw.c
    ntddraw/dd.c
    ntddraw/ddsurf.c
    ntddraw/d3d.c
    ntddraw/dvp.c
    ntddraw/mocomp.c
    ntddraw/eng.c
    ntddraw/dxeng.c
    ntuser/accelerator.c
    ntuser/callback.c
    ntuser/callproc.c
    ntuser/caret.c
    ntuser/class.c
    ntuser/clipboard.c
    ntuser/csr.c
    ntuser/cursoricon.c
    ntuser/defwnd.c
    ntuser/desktop.c
    ntuser/display.c
    ntuser/event.c
    ntuser/focus.c
    ntuser/guicheck.c
    ntuser/hook.c
    ntuser/hotkey.c
    ntuser/input.c
    ntuser/keyboard.c
    ntuser/kbdlayout.c
    ntuser/menu.c
    ntuser/message.c
    ntuser/metric.c
    ntuser/misc.c
    ntuser/monitor.c
    ntuser/mouse.c
    ntuser/msgqueue.c
    ntuser/ntstubs.c
    ntuser/ntuser.c
    ntuser/painting.c
    ntuser/prop.c
    ntuser/scrollbar.c
    ntuser/session.c
    ntuser/simplecall.c
    ntuser/sysparams.c
    ntuser/timer.c
    ntuser/useratom.c
    ntuser/vis.c
    ntuser/windc.c
    ntuser/window.c
    ntuser/winpos.c
    ntuser/winsta.c
    ntuser/object.c
    objects/arc.c
    objects/bezier.c
    objects/bitblt.c
    objects/bitmaps.c
    objects/brush.c
    objects/cliprgn.c
    objects/coord.c
    objects/dcattr.c
    objects/dclife.c
    objects/dcobjs.c
    objects/dcstate.c
    objects/dcutil.c
    objects/device.c
    objects/dibobj.c
    objects/drawing.c
    objects/fillshap.c
    objects/font.c
    objects/freetype.c
    objects/gdibatch.c
	objects/gdidbg.c
    objects/gdiobj.c
    objects/gdipool.c
    objects/icm.c
    objects/line.c
    objects/metafile.c
    objects/palette.c
    objects/path.c
    objects/pen.c
    objects/polyfill.c
    objects/print.c
    objects/rect.c
    objects/region.c
    objects/stockobj.c
    objects/text.c
    objects/wingl.c
    objects/xformobj.c
    stubs/stubs.c
    stubs/umpdstubs.c
    win32k.rc)

list(APPEND SOURCE ${GENDIB_FILES})

if(ARCH MATCHES i386)
list(APPEND SOURCE
    dib/i386/dib24bpp_hline.s
    dib/i386/dib32bpp_hline.s
    dib/i386/dib32bpp_colorfill.s
    eng/i386/floatobj.S
    misc/i386/cos_asm.s
    misc/i386/sin_asm.s
    misc/i386/atan2_asm.s
    misc/i386/floor_asm.s
    misc/i386/ceil_asm.s)
else()
list(APPEND SOURCE
    dib/dib24bppc.c
    dib/dib32bppc.c)
endif()

add_library(win32k SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/win32k.def
    ${SOURCE})

set_module_type(win32k kernelmodedriver)

target_link_libraries(win32k
    ${PSEH_LIB}
    dxguid
    libcntpr)

add_importlibs(win32k ntoskrnl hal ftfd)
add_pch(win32k pch.h)
add_cd_file(TARGET win32k DESTINATION reactos/system32 FOR all)
add_importlib_target(win32k.spec win32k.sys)

add_library(win32ksys sys-stubs.S)
set_source_files_properties(sys-stubs.S PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/w32ksvc.h)
