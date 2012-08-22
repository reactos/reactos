#define FUNC_VARS                               \
   struct pt_so_emit *so,                       \
   const struct draw_prim_info *input_prims,    \
   const struct draw_vertex_info *input_verts,  \
   unsigned start,                              \
   unsigned count

#define FUNC_ENTER                                                \
   /* declare more local vars */                                  \
   const unsigned prim = input_prims->prim;                       \
   const unsigned prim_flags = input_prims->flags;                \
   const boolean last_vertex_last = TRUE;                         \
   do {                                                           \
      debug_assert(input_prims->primitive_count == 1);            \
      switch (prim) {                                             \
      case PIPE_PRIM_LINES_ADJACENCY:                             \
      case PIPE_PRIM_LINE_STRIP_ADJACENCY:                        \
      case PIPE_PRIM_TRIANGLES_ADJACENCY:                         \
      case PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY:                    \
         debug_assert(!"unexpected primitive type in stream output"); \
         return;                                                  \
      default:                                                    \
         break;                                                   \
      }                                                           \
   } while (0)                                                    \

#define POINT(i0)                so_point(so,i0)
#define LINE(flags,i0,i1)        so_line(so,i0,i1)
#define TRIANGLE(flags,i0,i1,i2) so_tri(so,i0,i1,i2)

#include "draw_decompose_tmp.h"
