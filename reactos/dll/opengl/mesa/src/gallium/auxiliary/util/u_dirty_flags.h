#ifndef U_DIRTY_FLAGS_H
#define U_DIRTY_FLAGS_H

/* Here's a convenient list of dirty flags to use in a driver.  Either
 * include it directly or use it as a starting point for your own
 * list.
 */
#define U_NEW_VIEWPORT              0x1
#define U_NEW_RASTERIZER            0x2
#define U_NEW_FS                    0x4
#define U_NEW_FS_CONSTANTS          0x8
#define U_NEW_FS_SAMPLER_VIEW       0x10
#define U_NEW_FS_SAMPLER_STATES     0x20
#define U_NEW_VS                    0x40
#define U_NEW_VS_CONSTANTS          0x80
#define U_NEW_VS_SAMPLER_VIEW       0x100
#define U_NEW_VS_SAMPLER_STATES     0x200
#define U_NEW_BLEND                 0x400
#define U_NEW_CLIP                  0x800
#define U_NEW_SCISSOR               0x1000
#define U_NEW_POLYGON_STIPPLE       0x2000
#define U_NEW_FRAMEBUFFER           0x4000
#define U_NEW_VERTEX_ELEMENTS       0x8000
#define U_NEW_VERTEX_BUFFER         0x10000
#define U_NEW_QUERY                 0x20000
#define U_NEW_DEPTH_STENCIL         0x40000
#define U_NEW_GS                    0x80000
#define U_NEW_GS_CONSTANTS          0x100000
#define U_NEW_GS_SAMPLER_VIEW       0x200000
#define U_NEW_GS_SAMPLER_STATES     0x400000

#endif
