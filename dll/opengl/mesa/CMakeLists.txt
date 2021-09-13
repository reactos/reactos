
include_directories(.)

add_definitions(-DFAST_MATH -DTHREADS)

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE asm-386.S)
    add_definitions(-DUSE_ASM)
endif()

add_asm_files(mesa_asm ${ASM_SOURCE})

list(APPEND SOURCE
    accum.c
    alpha.c
    alphabuf.c
    api.c
    attrib.c
    bitmap.c
    blend.c
    clip.c
    colortab.c
    context.c
    copypix.c
    depth.c
    dlist.c
    drawpix.c
    enable.c
    eval.c
    feedback.c
    fog.c
    get.c
    hash.c
    image.c
    light.c
    lines.c
    logic.c
    masking.c
    matrix.c
    misc.c
    mmath.c
    pb.c
    pixel.c
    pointers.c
    points.c
    polygon.c
    quads.c
    rastpos.c
    readpix.c
    rect.c
    scissor.c
    shade.c
    span.c
    stencil.c
    teximage.c
    texobj.c
    texstate.c
    texture.c
    triangle.c
    varray.c
    vb.c
    vbfill.c
    vbrender.c
    vbxform.c
    xform.c
)

add_library(mesa STATIC ${SOURCE} ${mesa_asm})
add_dependencies(mesa psdk)


