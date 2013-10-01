
list(APPEND SOURCE
    s_aaline.c
    s_aatriangle.c
    s_alpha.c
    s_bitmap.c
    s_blend.c
    s_blit.c
    s_clear.c
    s_context.c
    s_copypix.c
    s_depth.c
    s_drawpix.c
    s_feedback.c
    s_fog.c
    s_fragprog.c
    s_lines.c
    s_logic.c
    s_masking.c
    s_points.c
    s_renderbuffer.c
    s_span.c
    s_stencil.c
    s_texcombine.c
    s_texfetch.c
    s_texfilter.c
    s_texrender.c
    s_texture.c
    s_triangle.c
    s_zoom.c
)

add_library(mesa_swrast STATIC ${SOURCE})