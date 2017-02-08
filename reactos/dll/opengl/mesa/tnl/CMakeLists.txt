
list(APPEND SOURCE
    t_context.c
    t_draw.c
    t_pipeline.c
    t_rasterpos.c
    t_vb_fog.c
    t_vb_light.c
    t_vb_normals.c
    t_vb_points.c
    t_vb_render.c
    t_vb_texgen.c
    t_vb_texmat.c
    t_vb_vertex.c
    t_vertex.c
    t_vertex_generic.c
    t_vertex_sse.c
    precomp.h)

add_library(mesa_tnl STATIC ${SOURCE})
add_dependencies(mesa_tnl xdk)
add_pch(mesa_tnl precomp.h SOURCE)
