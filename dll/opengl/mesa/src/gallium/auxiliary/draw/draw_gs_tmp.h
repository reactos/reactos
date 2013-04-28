#define FUNC_VARS struct draw_geometry_shader *gs,             \
                  const struct draw_prim_info *input_prims,    \
                  const struct draw_vertex_info *input_verts,  \
                  struct draw_prim_info *output_prims,         \
                  struct draw_vertex_info *output_verts

#define FUNC_ENTER                                                \
   /* declare more local vars */                                  \
   const unsigned prim = input_prims->prim;                       \
   const unsigned prim_flags = input_prims->flags;                \
   const unsigned count = input_prims->count;                     \
   const boolean last_vertex_last = TRUE;                         \
   do {                                                           \
      debug_assert(input_prims->primitive_count == 1);            \
      switch (prim) {                                             \
      case PIPE_PRIM_QUADS:                                       \
      case PIPE_PRIM_QUAD_STRIP:                                  \
      case PIPE_PRIM_POLYGON:                                     \
         debug_assert(!"unexpected primitive type in GS");        \
         return;                                                  \
      default:                                                    \
         break;                                                   \
      }                                                           \
   } while (0)                                                    \

#define POINT(i0)                             gs_point(gs,i0)
#define LINE(flags,i0,i1)                     gs_line(gs,i0,i1)
#define TRIANGLE(flags,i0,i1,i2)              gs_tri(gs,i0,i1,i2)
#define LINE_ADJ(flags,i0,i1,i2,i3)           gs_line_adj(gs,i0,i1,i2,i3)
#define TRIANGLE_ADJ(flags,i0,i1,i2,i3,i4,i5) gs_tri_adj(gs,i0,i1,i2,i3,i4,i5)

#include "draw_decompose_tmp.h"
