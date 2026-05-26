/*
 * PROJECT:     Original Xbox onboard hardware
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     nVidia NV2A (XGPU) header file
 * COPYRIGHT:   Copyright 2020 Stanislav Motylkov (x86corez@gmail.com)
 *
 * Extended with NV2A 2D engine, PFIFO, PMC, PRAMIN, RAMHT and PCRTC interrupt
 * definitions sourced from the public reverse-engineering work in the
 * nouveau/envytools project and the nxdk pbkit (MIT) reference implementation.
 */

#ifndef _XGPU_H_
#define _XGPU_H_

#pragma once

/* MMIO region layout (offsets from BAR0) ------------------------------------*/

#define NV2A_VIDEO_MEMORY_SIZE  (4 * 1024 * 1024) /* FIXME: obtain fb size from firmware somehow (Cromwell reserves high 4 MB of RAM) */

#define NV2A_PMC_OFFSET                0x000000
#define   NV2A_PMC_BOOT_0                (0x000 + NV2A_PMC_OFFSET)
#define   NV2A_PMC_INTR_0                (0x100 + NV2A_PMC_OFFSET)
#define   NV2A_PMC_INTR_EN_0             (0x140 + NV2A_PMC_OFFSET)
#define   NV2A_PMC_ENABLE                (0x200 + NV2A_PMC_OFFSET)
#define     NV2A_PMC_ENABLE_ALL_DISABLE    0x00000000
#define     NV2A_PMC_ENABLE_ALL_ENABLE     0xFFFFFFFF
#define     NV2A_PMC_ENABLE_PGRAPH         0x00001000
#define     NV2A_PMC_ENABLE_PFIFO          0x00000100
#define     NV2A_PMC_ENABLE_PTIMER         0x00010000
#define     NV2A_PMC_ENABLE_PVIDEO         0x01000000
#define     NV2A_PMC_ENABLE_PCRTC          0x02000000
#define     NV2A_PMC_ENABLE_PRAMDAC        0x10000000

#define NV2A_PFIFO_OFFSET              0x002000
#define   NV2A_PFIFO_RAMHT               (0x0210 + NV2A_PFIFO_OFFSET)
#define   NV2A_PFIFO_RAMFC               (0x0214 + NV2A_PFIFO_OFFSET)
#define   NV2A_PFIFO_INTR_0              (0x0100 + NV2A_PFIFO_OFFSET)
#define   NV2A_PFIFO_INTR_EN_0           (0x0140 + NV2A_PFIFO_OFFSET)
#define   NV2A_PFIFO_CACHES              (0x0500 + NV2A_PFIFO_OFFSET)
#define     NV2A_PFIFO_CACHES_REASSIGN     0x00000001
/* Per-channel run mode bitmask: bit N set => channel N is in DMA (push-buffer)
 * mode, clear => PIO mode.  The DMA pusher refuses to run for a channel whose
 * bit is clear, so this MUST be set before enabling CACHE1's DMA pusher. */
#define   NV2A_PFIFO_MODE                (0x0504 + NV2A_PFIFO_OFFSET)
#define   NV2A_PFIFO_CACHE1_PUSH0        (0x1200 + NV2A_PFIFO_OFFSET)
#define   NV2A_PFIFO_CACHE1_PUSH1        (0x1204 + NV2A_PFIFO_OFFSET)
#define     NV2A_PFIFO_CACHE1_PUSH1_MODE_DMA 0x00000100
#define   NV2A_PFIFO_CACHE1_DMA_PUSH     (0x1220 + NV2A_PFIFO_OFFSET)
#define     NV2A_PFIFO_CACHE1_DMA_PUSH_ENABLE 0x00000001
#define   NV2A_PFIFO_CACHE1_DMA_INSTANCE (0x122C + NV2A_PFIFO_OFFSET)
#define   NV2A_PFIFO_CACHE1_DMA_PUT      (0x1240 + NV2A_PFIFO_OFFSET)
#define   NV2A_PFIFO_CACHE1_DMA_GET      (0x1244 + NV2A_PFIFO_OFFSET)
#define   NV2A_PFIFO_CACHE1_PULL0        (0x1250 + NV2A_PFIFO_OFFSET)
#define     NV2A_PFIFO_CACHE1_PULL0_ENABLE 0x00000001

#define NV2A_FB_OFFSET                 0x100000
#define   NV2A_FB_CFG0                   (0x200 + NV2A_FB_OFFSET)

#define NV2A_PGRAPH_OFFSET             0x400000
#define   NV2A_PGRAPH_INTR               (0x0100 + NV2A_PGRAPH_OFFSET)
#define   NV2A_PGRAPH_INTR_EN            (0x0140 + NV2A_PGRAPH_OFFSET)
#define   NV2A_PGRAPH_CTX_USER           (0x0788 + NV2A_PGRAPH_OFFSET)
#define   NV2A_PGRAPH_FIFO               (0x0720 + NV2A_PGRAPH_OFFSET)
#define     NV2A_PGRAPH_FIFO_ENABLE        0x00000001

#define NV2A_PCRTC_OFFSET              0x600000
#define   NV2A_CRTC_INTR                 (0x0100 + NV2A_PCRTC_OFFSET)
#define     NV2A_CRTC_INTR_VBLANK          0x00000001
#define   NV2A_CRTC_INTR_EN              (0x0140 + NV2A_PCRTC_OFFSET)
#define     NV2A_CRTC_INTR_EN_VBLANK       0x00000001
#define   NV2A_CRTC_FRAMEBUFFER_START    (0x0800 + NV2A_PCRTC_OFFSET)
#define   NV2A_CRTC_REGISTER_INDEX      (0x13D4 + NV2A_PCRTC_OFFSET)
#define   NV2A_CRTC_REGISTER_VALUE      (0x13D5 + NV2A_PCRTC_OFFSET)

#define NV2A_RAMDAC_OFFSET             0x680000
#define   NV2A_RAMDAC_FP_HVALID_END      (0x838 + NV2A_RAMDAC_OFFSET)
#define   NV2A_RAMDAC_FP_VVALID_END      (0x818 + NV2A_RAMDAC_OFFSET)
#define   NV2A_USER_DAC_WRITE_MODE       (0x13C8 + NV2A_RAMDAC_OFFSET)
#define   NV2A_USER_DAC_PALETTE_DATA     (0x13C9 + NV2A_RAMDAC_OFFSET)

#define NV2A_PRAMIN_OFFSET             0x700000
#define NV2A_PRAMIN_SIZE                0x100000

#define NV2A_USER_OFFSET               0x800000
/* Submission window for channel <ch>, subchannel <sc>, method <m>:
 *   NV2A_USER_OFFSET + ch*0x10000 + sc*0x2000 + m   (PIO submission)
 * Subchannel is encoded by the SET_OBJECT (method 0x0) once per channel. */

/* PFIFO RAMIN-relative layout used by the accelerator -----------------------
 * We allocate RAMIN like this (offsets are relative to NV2A_PRAMIN_OFFSET):
 *   0x0000 .. 0x1FFF   RAMHT  (hash table, 8 KB)
 *   0x2000 .. 0x21FF   RAMFC  (per-channel FIFO context, 512 B)
 *   0x3000 .. 0x30FF   Object entries (NV04_CONTEXT_SURFACES_2D, NV_IMAGE_BLIT,
 *                      NV04_GDI_RECTANGLE_TEXT, NV01_NULL DMA contexts, ...)
 * The push buffer lives in the high end of VRAM, above the firmware
 * framebuffer.  See nv2a_accel.c for the exact slot map. */

/* Engine class IDs (the value put into the RAMHT object record) -------------*/
#define NV01_NULL                       0x00000030
/* NV2A-era 2D class IDs (NV10/NV11 numbering — the NV04 numbers 0x42/0x5F are
 * for RIVA TNT and are NOT what the NV2A PGRAPH / xemu decode).  GDI rectangle
 * (0x4A) has no NV2A class at all — xemu does not emulate it, so solid fills
 * stay on the CPU. */
#define NV04_CONTEXT_SURFACES_2D        0x00000062
#define NV04_GDI_RECTANGLE_TEXT         0x0000004A
#define NV_IMAGE_BLIT                   0x0000009F
#define NV01_CONTEXT_DMA                0x00000003

/* Methods we drive --------------------------------------------------------- */
#define NV_SET_OBJECT                                        0x00000000

#define NV04_SURFACES_2D_SET_DMA_NOTIFY                      0x00000180
#define NV04_SURFACES_2D_SET_DMA_IMAGE_SRC                   0x00000184
#define NV04_SURFACES_2D_SET_DMA_IMAGE_DST                   0x00000188
#define NV04_SURFACES_2D_FORMAT                              0x00000300
#define   NV04_SURFACES_2D_FORMAT_Y8                           0x01
#define   NV04_SURFACES_2D_FORMAT_R5G6B5                       0x04
#define   NV04_SURFACES_2D_FORMAT_X8R8G8B8_Z8R8G8B8            0x0B
#define   NV04_SURFACES_2D_FORMAT_A8R8G8B8                     0x0D
#define NV04_SURFACES_2D_PITCH                               0x00000304
#define NV04_SURFACES_2D_OFFSET_SRC                          0x00000308
#define NV04_SURFACES_2D_OFFSET_DST                          0x0000030C

#define NV_IMAGE_BLIT_SET_CONTEXT_SURFACES                   0x0000019C
#define NV_IMAGE_BLIT_OPERATION                              0x000002FC
#define   NV_IMAGE_BLIT_OPERATION_SRCCOPY                      0x00000003
#define NV_IMAGE_BLIT_POINT_IN                               0x00000300
#define NV_IMAGE_BLIT_POINT_OUT                              0x00000304
#define NV_IMAGE_BLIT_SIZE                                   0x00000308

#define NV04_GDI_SET_CONTEXT_SURFACE                         0x00000198
#define NV04_GDI_OPERATION                                   0x000002FC
#define   NV04_GDI_OPERATION_SRCCOPY                           0x00000003
#define NV04_GDI_FORMAT                                      0x00000300
#define   NV04_GDI_FORMAT_X16R5G6B5                            0x01
#define   NV04_GDI_FORMAT_X8R8G8B8                             0x03
#define NV04_GDI_SOLID_COLOR                                 0x000003FC
#define NV04_GDI_SOLID_RECT_POINT(i)                         (0x00000400 + (i) * 8)
#define NV04_GDI_SOLID_RECT_SIZE(i)                          (0x00000404 + (i) * 8)

/* Push-buffer command encoding --------------------------------------------- */
#define NV2A_METHOD(subch, method, count) \
    (((count) << 18) | ((subch) << 13) | ((method) & 0x1FFC))
#define NV2A_METHOD_NONINC(subch, method, count) \
    (0x40000000 | NV2A_METHOD(subch, method, count))

/* ------------------------------------------------------------------------- */
/* NV097 — Kelvin 3D engine (class 0x97) ------------------------------------ */
/* Method numbers and enum values verified against nxdk pbkit (nv_regs.h) and
 * its vp20/fp20 compiler output.  The NV2A has no usable fixed-function path:
 * a vertex program + register combiners are mandatory to rasterise anything. */
#define NV_KELVIN_PRIMITIVE                     0x00000097

#define NV097_NO_OPERATION                      0x00000100
#define NV097_SET_CONTEXT_DMA_COLOR             0x00000194
#define NV097_SET_CONTEXT_DMA_ZETA              0x00000198
#define NV097_SET_CONTEXT_DMA_VERTEX_A          0x0000019C
#define NV097_SET_SURFACE_CLIP_HORIZONTAL       0x00000200
#define NV097_SET_SURFACE_CLIP_VERTICAL         0x00000204
#define NV097_SET_SURFACE_FORMAT                0x00000208
#define NV097_SET_SURFACE_PITCH                 0x0000020C
#define NV097_SET_SURFACE_COLOR_OFFSET          0x00000210
#define NV097_SET_SURFACE_ZETA_OFFSET           0x00000214
#define NV097_SET_COMBINER_ALPHA_ICW            0x00000260
#define NV097_SET_COMBINER_SPECULAR_FOG_CW0     0x00000288
#define NV097_SET_COMBINER_SPECULAR_FOG_CW1     0x0000028C
#define NV097_SET_CONTROL0                      0x00000290
#define NV097_SET_FOG_ENABLE                    0x000002A4
#define NV097_SET_ALPHA_TEST_ENABLE             0x00000300
#define NV097_SET_BLEND_ENABLE                  0x00000304
#define NV097_SET_CULL_FACE_ENABLE              0x00000308
#define NV097_SET_DEPTH_TEST_ENABLE             0x0000030C
#define NV097_SET_DITHER_ENABLE                 0x00000310
#define NV097_SET_LIGHTING_ENABLE               0x00000314
#define NV097_SET_STENCIL_TEST_ENABLE           0x0000032C
#define NV097_SET_DEPTH_FUNC                    0x00000354
#define   NV097_SET_DEPTH_FUNC_ALWAYS             0x00000207
#define   NV097_SET_DEPTH_FUNC_LEQUAL             0x00000203
#define NV097_SET_COLOR_MASK                    0x00000358
#define NV097_SET_DEPTH_MASK                    0x0000035C
#define NV097_SET_SHADE_MODEL                   0x0000037C
#define   NV097_SET_SHADE_MODEL_FLAT              0x00001D00
#define   NV097_SET_SHADE_MODEL_SMOOTH            0x00001D01
#define NV097_SET_FRONT_POLYGON_MODE            0x0000038C
#define NV097_SET_BACK_POLYGON_MODE             0x00000390
#define   NV097_SET_POLYGON_MODE_POINT            0x00001B00
#define   NV097_SET_POLYGON_MODE_LINE             0x00001B01
#define   NV097_SET_POLYGON_MODE_FILL             0x00001B02
#define NV097_SET_CLIP_MIN                      0x00000394
#define NV097_SET_CLIP_MAX                      0x00000398
/* Stencil (Z24S8 zeta).  GL stencil func enums (0x200..0x207) and op enums
 * (KEEP 0x1E00, ZERO 0, REPLACE 0x1E01, INCR 0x1E02, DECR 0x1E03, INVERT 0x150A)
 * map 1:1 onto the NV2A values. */
#define NV097_SET_STENCIL_MASK                  0x00000360  /* stencil write mask */
#define NV097_SET_STENCIL_FUNC                  0x00000364
#define NV097_SET_STENCIL_FUNC_REF              0x00000368
#define NV097_SET_STENCIL_FUNC_MASK             0x0000036C  /* stencil compare mask */
#define NV097_SET_STENCIL_OP_FAIL               0x00000370
#define NV097_SET_STENCIL_OP_ZFAIL              0x00000374
#define NV097_SET_STENCIL_OP_ZPASS              0x00000378
/* Polygon offset (glPolygonOffset).  CORRECTED: the enables live at 0x330..0x338
 * and the slope/bias at 0x384/0x388 — the prior 0x370.. values aliased the
 * stencil-op registers above (so offset never actually took effect). */
#define NV097_SET_POLY_OFFSET_POINT_ENABLE      0x00000330
#define NV097_SET_POLY_OFFSET_LINE_ENABLE       0x00000334
#define NV097_SET_POLY_OFFSET_FILL_ENABLE       0x00000338
#define NV097_SET_POLYGON_OFFSET_SCALE_FACTOR   0x00000384  /* float: glPolygonOffset factor */
#define NV097_SET_POLYGON_OFFSET_BIAS           0x00000388  /* float: glPolygonOffset units  */
#define NV097_SET_SPECULAR_ENABLE               0x000003B8
#define NV097_SET_VIEWPORT_OFFSET               0x00000A20
#define NV097_SET_COMBINER_ALPHA_OCW            0x00000AA0
#define NV097_SET_COMBINER_COLOR_ICW            0x00000AC0
#define NV097_SET_VIEWPORT_SCALE                0x00000AF0
#define NV097_SET_COMPOSITE_MATRIX              0x00000680  /* fixed-function: 16 floats */
#define NV097_SET_SHADER_CLIP_PLANE_MODE        0x000017F8
#define NV097_SET_SHADER_STAGE_PROGRAM          0x00001E70
#define NV097_SET_SHADER_OTHER_STAGE_INPUT      0x00001E78
#define NV097_SET_TRANSFORM_PROGRAM             0x00000B00
#define NV097_SET_TRANSFORM_CONSTANT            0x00000B80
#define NV097_SET_POINT_SIZE                    0x0000043C  /* fixed-point: size * 8 (ignored in program mode) */
#define NV097_SET_PTSIZE_4F                     0x00001A60  /* SET_VERTEX_DATA4F_M + 6*16 (attr PTSIZE); program-mode point size */
#define NV097_SET_VERTEX4F                      0x00001518
#define NV097_SET_DIFFUSE_COLOR4F               0x00001550
#define NV097_SET_VERTEX_DATA_ARRAY_OFFSET      0x00001720  /* +attr*4; GPU offset */
#define NV097_SET_VERTEX_DATA_ARRAY_FORMAT      0x00001760  /* +attr*4 */
#define   NV097_VTXFMT_TYPE_F                     0x00000002  /* float */
#define NV097_DRAW_ARRAYS                       0x00001810  /* ((count-1)<<24)|start */
#define NV097_INLINE_ARRAY                      0x00001818  /* non-incrementing */
#define NV2A_VTX_ATTR_POSITION                  0
#define NV2A_VTX_ATTR_DIFFUSE                   3
#define NV097_SET_BEGIN_END                     0x000017FC
#define   NV097_SET_BEGIN_END_OP_END              0x00
#define   NV097_SET_BEGIN_END_OP_POINTS           0x01
#define   NV097_SET_BEGIN_END_OP_LINES            0x02
#define   NV097_SET_BEGIN_END_OP_LINE_LOOP        0x03
#define   NV097_SET_BEGIN_END_OP_LINE_STRIP       0x04
#define   NV097_SET_BEGIN_END_OP_TRIANGLES        0x05
#define NV097_SET_CONTEXT_DMA_A                 0x00000184  /* texture DMA context */
#define NV097_SET_BLEND_FUNC_SFACTOR            0x00000344  /* == GL blend enum (incl. CONSTANT_* 0x8001..0x8004) */
#define NV097_SET_BLEND_FUNC_DFACTOR            0x00000348
#define NV097_SET_BLEND_COLOR                   0x0000034C  /* packed 0xAARRGGBB constant blend colour */
#define NV097_SET_BLEND_EQUATION                0x00000350
/* Blend-equation values: ADD/MIN/MAX share the GL enums (0x8006/7/8); GL's
 * FUNC_SUBTRACT (0x800A) / FUNC_REVERSE_SUBTRACT (0x800B) map to these. */
#define   NV097_SET_BLEND_EQUATION_V_FUNC_ADD              0x00008006
#define   NV097_SET_BLEND_EQUATION_V_MIN                   0x00008007
#define   NV097_SET_BLEND_EQUATION_V_MAX                   0x00008008
#define   NV097_SET_BLEND_EQUATION_V_FUNC_SUBTRACT         0x0000845A
#define   NV097_SET_BLEND_EQUATION_V_FUNC_REVERSE_SUBTRACT 0x0000845B
#define NV097_SET_TEXTURE_OFFSET                0x00001B00  /* +0x40*stage */
#define NV097_SET_TEXTURE_FORMAT                0x00001B04
#define   NV097_TEXFMT_COLOR_LU_IMAGE_A8R8G8B8    0x12        /* linear ARGB8888 */
#define NV097_SET_TEXTURE_ADDRESS               0x00001B08
#define NV097_SET_TEXTURE_CONTROL0              0x00001B0C  /* +0x40*stage */
#define   NV097_SET_TEXTURE_CONTROL0_ENABLE       (1u << 30)
#define NV097_SET_TEXTURE_CONTROL1              0x00001B10  /* IMAGE_PITCH<<16 */
#define NV097_SET_TEXTURE_FILTER                0x00001B14
#define NV097_SET_TEXTURE_IMAGE_RECT            0x00001B1C  /* (W<<16)|H */
#define NV097_SET_TEXCOORD0_2F                  0x000018C8  /* SET_VERTEX_DATA2F_M + 9*8 (attr TEX0) */
#define NV097_SET_TEXCOORD0_4F                  0x00001A90  /* SET_VERTEX_DATA4F_M + 9*16 (attr TEX0, 4 floats) */
#define NV097_SET_COMBINER_COLOR_OCW            0x00001E40
#define NV097_SET_COMBINER_CONTROL              0x00001E60
#define NV097_SET_COMBINER_FACTOR0              0x00000A60  /* per-stage constant colour 0 (GL_TEXTURE_ENV_COLOR for BLEND) */
#define NV097_SET_TRANSFORM_EXECUTION_MODE      0x00001E94
#define   NV097_TRANSFORM_EXECUTION_MODE_PROGRAM  0x00000006 /* PROGRAM|RANGE_PRIV */
#define NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN 0x00001E98
#define NV097_SET_TRANSFORM_PROGRAM_LOAD        0x00001E9C
#define NV097_SET_TRANSFORM_PROGRAM_START       0x00001EA0
#define NV097_SET_TRANSFORM_CONSTANT_LOAD       0x00001EA4
#define NV097_SET_ZSTENCIL_CLEAR_VALUE          0x00001D8C
#define NV097_SET_COLOR_CLEAR_VALUE             0x00001D90
#define NV097_CLEAR_SURFACE                     0x00001D94
#define   NV097_CLEAR_SURFACE_COLOR               0x000000F0  /* R|G|B|A */
#define   NV097_CLEAR_SURFACE_Z_STENCIL           0x00000003
#define NV097_SET_CLEAR_RECT_HORIZONTAL         0x00001D98
#define NV097_SET_CLEAR_RECT_VERTICAL           0x00001D9C

#endif /* _XGPU_H_ */
