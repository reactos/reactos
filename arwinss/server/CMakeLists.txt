spec2def(win32k.sys win32k.spec ADD_IMPORTLIB)

include_directories(
    .
    include
    ${REACTOS_SOURCE_DIR}/win32ss/include
    ${REACTOS_SOURCE_DIR}/ntoskrnl/include
    ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/include
    ${REACTOS_SOURCE_DIR}/include/reactos/subsys
    ${REACTOS_SOURCE_DIR}/include/reactos/drivers
    ${REACTOS_SOURCE_DIR}/arwinss/include)

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
    dib/dib1bpp.c
    dib/dib4bpp.c
    dib/dib8bpp.c
    dib/dib16bpp.c
    dib/dib24bpp.c
    dib/dib32bpp.c
    dib/floodfill.c
    dib/stretchblt.c
    eng/device.c
    eng/driver.c
    eng/engblt.c
    eng/engbrush.c
    eng/engclip.c
    eng/engdev.c
    eng/engdrv.c
    eng/engevent.c
    eng/engerror.c
    eng/engfile.c
    eng/engfloat.c
    eng/engfont.c
    eng/engmem.c
    eng/engmisc.c
    eng/engpaint.c
    eng/engpal.c
    eng/engpath.c
    eng/engpointer.c
    eng/engprint.c
    eng/engquery.c
    eng/engrtl.c
    eng/engsem.c
    eng/engsurf.c
    eng/engtext.c
    eng/engwnd.c
    eng/engxform.c
    eng/engxlate.c
    gdi/bitmap.c
    gdi/dc.c
    gdi/enum.c
    gdi/misc.c
    gre/arc.c
    gre/bitblt.c
    gre/brushobj.c
    gre/clipobj.c
    gre/drawing.c
    gre/ellipse.c
    gre/font.c
    gre/gdiobj.c
    gre/lineto.c
    gre/pen.c
    gre/polyfill.c
    gre/rect.c
    gre/surfobj.c
    main/csr.c
    main/err.c
    main/init.c
    main/cursor.c
    main/display.c
    main/monitor.c
    main/kbdlayout.c
    main/keyboard.c
    main/misc.c
    main/shutdown.c
    swm/winman.c
    wine/atom.c
    wine/class.c
    wine/clipboard.c
    wine/directory.c
    wine/handle.c
    wine/hook.c
    wine/main.c
    wine/object.c
    wine/process.c
    wine/queue.c
    wine/region.c
    wine/stubs.c
    wine/timeout.c
    wine/user.c
    wine/window.c
    wine/winesup.c
    wine/winstation.c
    win32k.rc)

list(APPEND SOURCE
    dib/dib.c
    ${GENDIB_FILES})

if(ARCH MATCHES i386)
list(APPEND SOURCE
    dib/i386/dib24bpp_hline.s
    dib/i386/dib32bpp_hline.s
    dib/i386/dib32bpp_colorfill.s
    eng/i386/floatobj.S
    math/i386/cos_asm.s
    math/i386/sin_asm.s
    math/i386/atan2_asm.s
    math/i386/floor_asm.s
    math/i386/ceil_asm.s)
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

add_importlibs(win32k ntoskrnl hal)
#add_pch(win32k pch.h SOURCE)
add_cd_file(TARGET win32k DESTINATION reactos/system32 FOR all)

set_source_files_properties(sys-stubs.S PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/w32ksvc.h)
add_asm_files(win32ksys_asm sys-stubs.S)
add_library(win32ksys ${win32ksys_asm})
set_target_properties(win32ksys PROPERTIES LINKER_LANGUAGE "C")
