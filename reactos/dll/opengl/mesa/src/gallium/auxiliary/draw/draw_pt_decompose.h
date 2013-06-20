#define LOCAL_VARS                           \
   char *verts = (char *) vertices;          \
   const boolean last_vertex_last =          \
      !(draw->rasterizer->flatshade &&       \
        draw->rasterizer->flatshade_first);

#include "draw_decompose_tmp.h"
