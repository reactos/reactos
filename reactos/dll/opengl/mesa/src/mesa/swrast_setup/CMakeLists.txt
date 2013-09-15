
list(APPEND SOURCE
    ss_context.c
    ss_triangle.c
)

add_library(mesa_swrast_setup STATIC ${SOURCE})