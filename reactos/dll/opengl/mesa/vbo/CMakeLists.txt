
list(APPEND SOURCE
    vbo_context.c
    vbo_exec.c
    vbo_exec_api.c
    vbo_exec_array.c
    vbo_exec_draw.c
    vbo_exec_eval.c
    vbo_noop.c
    vbo_rebase.c
    vbo_split.c
    vbo_split_copy.c
    vbo_split_inplace.c
    vbo_save.c
    vbo_save_api.c
    vbo_save_draw.c
    vbo_save_loopback.c
    precomp.h)

add_library(mesa_vbo STATIC ${SOURCE})
add_dependencies(mesa_vbo xdk)
add_pch(mesa_vbo precomp.h SOURCE)
