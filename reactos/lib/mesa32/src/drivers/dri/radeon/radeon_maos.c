

/* If using new packets, can choose either verts or arrays.
 * Otherwise, must use verts.
 */
#include "radeon_context.h"
#define RADEON_MAOS_VERTS 1
#if (RADEON_MAOS_VERTS) || (RADEON_OLD_PACKETS)
#include "radeon_maos_verts.c"
#else
#include "radeon_maos_arrays.c"
#endif
