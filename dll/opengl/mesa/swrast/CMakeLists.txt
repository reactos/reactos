
list(APPEND SOURCE
    s_aaline.c
    s_aatriangle.c
    s_alpha.c
    s_bitmap.c
    s_blend.c
    s_clear.c
    s_context.c
    s_copypix.c
    s_depth.c
    s_drawpix.c
    s_feedback.c
    s_fog.c
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
    s_texture.c
    s_triangle.c
    s_zoom.c
    precomp.h)

add_library(mesa_swrast STATIC ${SOURCE})
add_dependencies(mesa_swrast xdk)
add_pch(mesa_swrast precomp.h SOURCE)

if(MSVC AND (NOT USE_CLANG_CL))
    target_link_options(mesa_swrast PRIVATE "/we4189" " ")
else()
    target_link_options(mesa_swrast PRIVATE "-Wno-unused-variable")
    if(USE_CLANG_CL)
        target_link_options(mesa_swrast PRIVATE "-Wno-sometimes-uninitialized, -Wno-unused-local-typedef")
    endif()
endif()
