
list(APPEND SOURCE
    api_arrayelt.c
    api_exec.c
    api_loopback.c
    api_validate.c
    accum.c
    attrib.c
    blend.c
    bufferobj.c
    buffers.c
    clear.c
    clip.c
    context.c
    cpuinfo.c
    depth.c
    dlist.c
    dlopen.c
    drawpix.c
    enable.c
    enums.c
    eval.c
    execmem.c
    extensions.c
    feedback.c
    fog.c
    formats.c
    format_pack.c
    format_unpack.c
    framebuffer.c
    get.c
    getstring.c
    hash.c
    hint.c
    image.c
    imports.c
    light.c
    lines.c
    matrix.c
    mm.c
    multisample.c
    pack.c
    pixel.c
    pixelstore.c
    pixeltransfer.c
    points.c
    polygon.c
    rastpos.c
    readpix.c
    renderbuffer.c
    scissor.c
    shared.c
    state.c
    stencil.c
    texenv.c
    texformat.c
    texgen.c
    texgetimage.c
    teximage.c
    texobj.c
    #texpal.c
    texparam.c
    texstate.c
    texstorage.c
    texstore.c
    varray.c
    version.c
    viewport.c
    vtxfmt.c
    precomp.h)

add_library(mesa_main STATIC ${SOURCE})
add_dependencies(mesa_main xdk)
add_pch(mesa_main precomp.h SOURCE)

if(NOT MSVC)
    add_target_compile_flags(mesa_main "-Wno-type-limits")
elseif(USE_CLANG_CL)
    add_target_compile_flags(mesa_main "-Wno-cast-calling-convention -Wno-unused-local-typedef")
    add_target_compile_flags(mesa_main "-Wno-tautological-unsigned-zero-compare -Wno-constant-conversion")
endif()
