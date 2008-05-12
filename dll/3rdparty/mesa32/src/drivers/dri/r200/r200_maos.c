

/* Currently, can only use arrays, verts are not implemented, though
 * verts are suspected to be faster.
 * To get an idea how the verts path works, look at the radeon implementation.
 */
#include <string.h>
 
#include "r200_context.h"
#define R200_MAOS_VERTS 0
#if (R200_MAOS_VERTS)
#include "r200_maos_verts.c"
#else
#include "r200_maos_arrays.c"
#endif
