
set(USE_DIBLIB FALSE)

# Give WIN32 subsystem its own project.
PROJECT(WIN32SS)

add_subdirectory(drivers)

if(USE_DIBLIB)
    add_subdirectory(gdi/diblib)
endif()

add_subdirectory(gdi/gdi32)
add_subdirectory(gdi/gdi32_vista)
add_subdirectory(printing)
add_subdirectory(reactx)
add_subdirectory(user/user32)
add_subdirectory(user/winsrv)

spec2def(win32k.sys win32k.spec ADD_IMPORTLIB)

# embed RTC libs
if (STACK_PROTECTOR)
    target_sources(libwin32k PRIVATE $<TARGET_OBJECTS:gcc_ssp_win32k>)
endif()

include_directories(
    .
    include
    ${REACTOS_SOURCE_DIR}/ntoskrnl/include
    ${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/freetype/include
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers)

add_definitions(
    -DLANGPACK
    -D_WIN32K_)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/gdi/dib)

list(APPEND GENDIB_FILES
    ${CMAKE_CURRENT_BINARY_DIR}/gdi/dib/dib8gen.c
    ${CMAKE_CURRENT_BINARY_DIR}/gdi/dib/dib16gen.c
    ${CMAKE_CURRENT_BINARY_DIR}/gdi/dib/dib32gen.c)

add_custom_command(
    OUTPUT ${GENDIB_FILES}
    COMMAND native-gendib ${CMAKE_CURRENT_BINARY_DIR}/gdi/dib
    DEPENDS native-gendib)

list(APPEND SOURCE
    gdi/dib/alphablend.c
    gdi/dib/dib1bpp.c
    gdi/dib/dib4bpp.c
    gdi/dib/dib8bpp.c
    gdi/dib/dib16bpp.c
    gdi/dib/dib24bpp.c
    gdi/dib/dib32bpp.c
    gdi/dib/floodfill.c
    gdi/dib/stretchblt.c
    gdi/eng/alphablend.c
    gdi/eng/bitblt.c
    gdi/eng/engbrush.c
    gdi/eng/engevent.c
    gdi/eng/clip.c
    gdi/eng/debug.c
    gdi/eng/device.c
    gdi/eng/driverobj.c
    gdi/eng/error.c
    gdi/eng/float.c
    gdi/eng/gradient.c
    gdi/eng/lineto.c
    gdi/eng/ldevobj.c
    gdi/eng/mapping.c
    gdi/eng/math.c
    gdi/eng/mdevobj.c
    gdi/eng/mem.c
    gdi/eng/engmisc.c
    gdi/eng/mouse.c
    gdi/eng/multidisp.c
    gdi/eng/paint.c
    gdi/eng/pathobj.c
    gdi/eng/pdevobj.c
    gdi/eng/perfcnt.c
    gdi/eng/rlecomp.c
    gdi/eng/semaphor.c
    gdi/eng/sort.c
    gdi/eng/string.c
    gdi/eng/stretchblt.c
    gdi/eng/surface.c
    gdi/eng/transblt.c
    gdi/eng/engwindow.c
    gdi/eng/xlateobj.c
    user/ntuser/main.c
    user/ntuser/misc/file.c
    user/ntuser/misc/rtlstr.c
    user/ntuser/misc/copy.c
    user/ntuser/misc/registry.c
    reactx/ntddraw/ddraw.c
    reactx/ntddraw/dd.c
    reactx/ntddraw/ddsurf.c
    reactx/ntddraw/d3d.c
    reactx/ntddraw/d3dkmt.c
    reactx/ntddraw/dvp.c
    reactx/ntddraw/mocomp.c
    reactx/ntddraw/eng.c
    reactx/ntddraw/dxeng.c
    user/ntuser/accelerator.c
    user/ntuser/callback.c
    user/ntuser/callproc.c
    user/ntuser/caret.c
    user/ntuser/class.c
    user/ntuser/clipboard.c
    user/ntuser/console.c
    user/ntuser/csr.c
    user/ntuser/cursoricon.c
    user/ntuser/dde.c
    user/ntuser/defwnd.c
    user/ntuser/desktop.c
    user/ntuser/display.c
    user/ntuser/draw.c
    user/ntuser/event.c
    user/ntuser/focus.c
    user/ntuser/ghost.c
    user/ntuser/guicheck.c
    user/ntuser/hook.c
    user/ntuser/hotkey.c
    user/ntuser/input.c
    user/ntuser/ime.c
    user/ntuser/keyboard.c
    user/ntuser/kbdlayout.c
    user/ntuser/layered.c
    user/ntuser/menu.c
    user/ntuser/message.c
    user/ntuser/metric.c
    user/ntuser/misc.c
    user/ntuser/monitor.c
    user/ntuser/mouse.c
    user/ntuser/msgqueue.c
    user/ntuser/nonclient.c
    user/ntuser/ntstubs.c
    user/ntuser/ntuser.c
    user/ntuser/painting.c
    user/ntuser/prop.c
    user/ntuser/scrollbar.c
    user/ntuser/scrollex.c
    user/ntuser/session.c
    user/ntuser/shutdown.c
    user/ntuser/simplecall.c
    user/ntuser/sysparams.c
    user/ntuser/timer.c
    user/ntuser/useratom.c
    user/ntuser/usrheap.c
    user/ntuser/vis.c
    user/ntuser/windc.c
    user/ntuser/window.c
    user/ntuser/winpos.c
    user/ntuser/winsta.c
    user/ntuser/object.c
    user/rtl/text.c
    gdi/ntgdi/arc.c
    gdi/ntgdi/bezier.c
    gdi/ntgdi/bitblt.c
    gdi/ntgdi/bitmaps.c
    gdi/ntgdi/cliprgn.c
    gdi/ntgdi/coord.c
    gdi/ntgdi/dcattr.c
    gdi/ntgdi/dclife.c
    gdi/ntgdi/dcobjs.c
    gdi/ntgdi/dcstate.c
    gdi/ntgdi/dcutil.c
    gdi/ntgdi/device.c
    gdi/ntgdi/dibobj.c
    gdi/ntgdi/drawing.c
    gdi/ntgdi/fillshap.c
    gdi/ntgdi/font.c
    gdi/ntgdi/freetype.c
    gdi/ntgdi/gdibatch.c
    gdi/ntgdi/gdidbg.c
    gdi/ntgdi/gdiobj.c
    gdi/ntgdi/gdipool.c
    gdi/ntgdi/icm.c
    gdi/ntgdi/init.c
    gdi/ntgdi/line.c
    gdi/ntgdi/metafile.c
    gdi/ntgdi/palette.c
    gdi/ntgdi/path.c
    gdi/ntgdi/pen.c
    gdi/ntgdi/polyfill.c
    gdi/ntgdi/print.c
    gdi/ntgdi/rect.c
    gdi/ntgdi/region.c
    gdi/ntgdi/stockobj.c
    gdi/ntgdi/text.c
    gdi/ntgdi/wingl.c
    gdi/ntgdi/xformobj.c
    gdi/eng/stubs.c
    gdi/eng/umpdstubs.c
    pch.h)

if(USE_DIBLIB)
    add_definitions(-D_USE_DIBLIB_)
    list(APPEND SOURCE
        gdi/dib/dib_new.c
        gdi/eng/bitblt_new.c)
else()
    list(APPEND SOURCE
        gdi/dib/dib.c
        gdi/eng/copybits.c
        ${GENDIB_FILES})
endif()

if(ARCH STREQUAL "i386")
list(APPEND ASM_SOURCE
    gdi/dib/i386/dib24bpp_hline.s
    gdi/dib/i386/dib32bpp_hline.s
    gdi/dib/i386/dib32bpp_colorfill.s
    gdi/eng/i386/floatobj.S)
list(APPEND SOURCE gdi/eng/i386/floatobj.c)
else()
list(APPEND SOURCE
    gdi/dib/dib24bppc.c
    gdi/dib/dib32bppc.c)
endif()

if(KDBG)
    list(APPEND SOURCE gdi/ntgdi/gdikdbgext.c)
endif()

add_asm_files(win32k_asm ${ASM_SOURCE})

add_library(win32k MODULE
    ${SOURCE}
    gdi/ntgdi/brush.cpp
    ${win32k_asm}
    win32k.rc
    ${CMAKE_CURRENT_BINARY_DIR}/win32k.def)

set_module_type(win32k kernelmodedriver)
target_link_libraries(win32k ${PSEH_LIB} dxguid libcntpr)

if(USE_DIBLIB)
    target_link_libraries(win32k diblib)
endif()

add_importlibs(win32k ntoskrnl hal ftfd)
add_pch(win32k pch.h SOURCE)
add_cd_file(TARGET win32k DESTINATION reactos/system32 FOR all)

set_source_files_properties(sys-stubs.S PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/w32ksvc.h)
add_asm_files(win32ksys_asm sys-stubs.S)
add_library(win32ksys ${win32ksys_asm})
set_target_properties(win32ksys PROPERTIES LINKER_LANGUAGE "C")
add_dependencies(win32ksys asm)
