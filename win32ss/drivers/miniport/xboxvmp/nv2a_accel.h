/*
 * PROJECT:     ReactOS Xbox miniport video driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     NVIDIA NV2A 2D engine accelerator — shared interface
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

#ifndef _XBOX_NV2A_ACCEL_H_
#define _XBOX_NV2A_ACCEL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* IOCTL surface — always defined ----------------------------------------- */
/* ------------------------------------------------------------------------- */

#ifndef CTL_CODE
/* Fallback for callers that only include winioctl.h's bare minimum. */
#include <winioctl.h>
#endif

#ifndef FILE_DEVICE_VIDEO
#define FILE_DEVICE_VIDEO 0x00000023
#endif

#define IOCTL_VIDEO_NV2A_FILL_RECT \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_NV2A_SCREEN_BLT \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_NV2A_QUERY_CAPS \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VIDEO_NV2A_DRAW_3D \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x903, METHOD_BUFFERED, FILE_ANY_ACCESS)
/* General VRAM<->VRAM blit between arbitrary surfaces (screen + offscreen device
 * bitmaps), addressed by absolute GPU offset + pitch.  Powers DrvCreateDeviceBitmap
 * caching: GDI keeps bitmaps in VRAM and we blit them to/from the screen on the
 * NV2A 2D engine. */
#define IOCTL_VIDEO_NV2A_BLT_EX \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x904, METHOD_BUFFERED, FILE_ANY_ACCESS)
/* Upload a linear A8R8G8B8 texture into the VRAM texture heap.  Input is an
 * NV2A_TEX_UPLOAD header + Width*Height ARGB pixels; output is the assigned GPU
 * offset (ULONG) the ICD then references in NV2A_DRAW_3D.TexOffset. */
#define IOCTL_VIDEO_NV2A_TEX_UPLOAD \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x905, METHOD_BUFFERED, FILE_ANY_ACCESS)
/* Read back a rectangle of the offscreen colour surface (the rendered frame) into
 * a CPU buffer — powers glReadPixels / glCopyTexImage / glCopyPixels.  Input is an
 * NV2A_READBACK header; output is Width*Height ARGB pixels (top-down). */
#define IOCTL_VIDEO_NV2A_READBACK \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x906, METHOD_BUFFERED, FILE_ANY_ACCESS)
/* Write a rectangle of ARGB pixels into the offscreen colour surface — powers
 * glDrawPixels / glBitmap / glCopyPixels.  Input is an NV2A_WRITEBACK header
 * followed by Width*Height ARGB pixels (top-down). */
#define IOCTL_VIDEO_NV2A_WRITEBACK \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x907, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* Private GDI ExtEscape code: the user-mode xboxogl ICD can't call the video
 * IOCTLs directly, so it ships a clip-space triangle batch (NV2A_DRAW_3D) to
 * the xboxdisp display driver via ExtEscape; DrvEscape forwards it to
 * IOCTL_VIDEO_NV2A_DRAW_3D. TODO: I do wonder if this is how other GPU drivers do this! */
#define XBOX_ESC_NV2A_DRAW3D  0x00004E33
/* Private GDI ExtEscape code to upload a texture (forwarded to
 * IOCTL_VIDEO_NV2A_TEX_UPLOAD).  pvOut receives the assigned GPU offset. */
#define XBOX_ESC_NV2A_TEXUPLOAD 0x00004E34
/* Read back / write the offscreen colour surface (glReadPixels, glCopyTex, glDrawPixels). */
#define XBOX_ESC_NV2A_READBACK  0x00004E35
#define XBOX_ESC_NV2A_WRITEBACK 0x00004E36

typedef struct _NV2A_FILL_RECT
{
    unsigned long X;
    unsigned long Y;
    unsigned long Width;
    unsigned long Height;
    unsigned long Color;     /* in the surface's native format */
} NV2A_FILL_RECT, *PNV2A_FILL_RECT;

typedef struct _NV2A_SCREEN_BLT
{
    unsigned long SrcX;
    unsigned long SrcY;
    unsigned long DstX;
    unsigned long DstY;
    unsigned long Width;
    unsigned long Height;
} NV2A_SCREEN_BLT, *PNV2A_SCREEN_BLT;

typedef struct _NV2A_BLT_EX
{
    unsigned long SrcOffset;   /* absolute GPU/VRAM byte offset of the source surface base */
    unsigned long SrcPitch;    /* source stride in bytes */
    unsigned long SrcX;
    unsigned long SrcY;
    unsigned long DstOffset;   /* absolute GPU/VRAM byte offset of the dest surface base */
    unsigned long DstPitch;
    unsigned long DstX;
    unsigned long DstY;
    unsigned long Width;
    unsigned long Height;
} NV2A_BLT_EX, *PNV2A_BLT_EX;

typedef struct _NV2A_CAPS
{
    unsigned long StructSize;
    unsigned long HardwareAccelEnabled;  /* TRUE if the GPU is driving the rasteriser */
    unsigned long PushBufferSize;
    unsigned long PushBufferGpuOffset;
    unsigned long SurfacePitch;
    unsigned long SurfaceFormat;         /* NV04_SURFACES_2D_FORMAT_* */
    /* Offscreen device-bitmap heap: a slice of VRAM above the visible framebuffer
     * (below the 3D scratch region + pushbuffer) the display driver can carve up
     * for VRAM-resident GDI bitmaps.  FrameBufferGpuOffset lets the display driver
     * translate a CPU framebuffer pointer to an absolute GPU offset for BLT_EX. */
    unsigned long FrameBufferGpuOffset;  /* GPU offset of the mapped framebuffer base */
    unsigned long OffscreenHeapStart;    /* absolute GPU offset where the free heap begins */
    unsigned long OffscreenHeapSize;     /* bytes available for device bitmaps (0 = none) */
} NV2A_CAPS, *PNV2A_CAPS;

/* One vertex for the NV2A 3D (Kelvin) triangle path.  Position is CLIP space
 * (the caller applies its modelview/projection; the GPU does the perspective
 * divide + viewport).  Colour is float [0,1] RGBA fed to DIFFUSE_COLOR4F. */
typedef struct _NV2A_3D_VERTEX
{
    float x, y, z, w;
    float r, g, b, a;
    /* Texture coords carried for HARDWARE perspective correction: u,v are the
     * texel coords already PREMULTIPLIED by q=1/clip_w, and q is sent as the
     * texcoord's homogeneous component.  The NV2A's PROJECT2D textureProj divides
     * u,v by the interpolated q, recovering perspective-correct coords even though
     * our screen-space vertices interpolate affinely. */
    float u, v, q;
} NV2A_3D_VERTEX, *PNV2A_3D_VERTEX;

/* Texture upload payload: this header is followed by Width*Height ULONG ARGB
 * pixels.  Sent via IOCTL_VIDEO_NV2A_TEX_UPLOAD / XBOX_ESC_NV2A_TEXUPLOAD. */
typedef struct _NV2A_TEX_UPLOAD
{
    unsigned long Width;
    unsigned long Height;
    /* If non-zero, the GPU offset of a PRIOR upload of the SAME dimensions: the
     * miniport overwrites those texels in place and returns the same offset,
     * instead of bump-allocating a fresh slot.  The ICD passes its cached
     * gpuOffset here so re-specifying / glTexSubImage2D doesn't leak the heap.
     * 0 = allocate a new slot. */
    unsigned long ExistingOffset;
    /* unsigned long Pixels[Width*Height]; — ARGB8888, row-major, top-down */
} NV2A_TEX_UPLOAD, *PNV2A_TEX_UPLOAD;

/* Readback: copy a Width*Height rect at (X,Y) of the offscreen colour surface
 * (screen pixels, top-down) into the output buffer as ARGB DWORDs. */
typedef struct _NV2A_READBACK
{
    unsigned long X, Y, Width, Height;
} NV2A_READBACK, *PNV2A_READBACK;

/* Writeback: this header is followed by Width*Height ARGB DWORDs (top-down) that
 * are written to (X,Y) of the offscreen colour surface.  Flags bit0 = skip pixels
 * whose alpha is 0 (glBitmap mask). */
#define NV2A_WRITEBACK_FLAG_SKIP_TRANSPARENT 0x00000001
typedef struct _NV2A_WRITEBACK
{
    unsigned long X, Y, Width, Height, Flags;
    /* unsigned long Pixels[Width*Height]; — ARGB8888, top-down */
} NV2A_WRITEBACK, *PNV2A_WRITEBACK;

#define NV2A_3D_MAX_VERTS 60   /* multiple of 3; keeps the payload small */

/* NV2A_DRAW_3D.Topology values (== NV097_SET_BEGIN_END_OP_*). */
#define NV2A_TOPO_POINTS      1
#define NV2A_TOPO_LINES       2
#define NV2A_TOPO_LINE_LOOP   3
#define NV2A_TOPO_LINE_STRIP  4
#define NV2A_TOPO_TRIANGLES   5

/* Frame control flags: the ICD clears once at the start of a frame, submits any
 * number of triangle batches into the persistent offscreen surface, then presents
 * once at the end.  Without these every batch would clear + present, so only the
 * last primitive of a multi-primitive frame would survive. */
#define NV2A_DRAW3D_FLAG_CLEAR        0x00000001  /* clear the dst rect colour first (ClearColor) */
#define NV2A_DRAW3D_FLAG_PRESENT      0x00000002  /* blit the dst rect to the visible fb after */
#define NV2A_DRAW3D_FLAG_CLEAR_DEPTH  0x00000004  /* clear the depth (Z) buffer */
#define NV2A_DRAW3D_FLAG_CLEAR_STENCIL 0x00000008 /* clear the stencil buffer (Z24S8 only) */

typedef struct _NV2A_DRAW_3D
{
    unsigned long  VertexCount;  /* number of vertices (0 for clear/present-only) */
    unsigned long  Topology;     /* NV097_SET_BEGIN_END_OP_* (POINTS/LINES/.../TRIANGLES) */
    unsigned long  Flags;        /* NV2A_DRAW3D_FLAG_* */
    unsigned long  ClearColor;   /* packed 0x00RRGGBB used when FLAG_CLEAR is set */
    unsigned long  DepthTestEnable; /* 0/1 — enable the hardware depth test for this batch */
    unsigned long  DepthFunc;       /* NV097_SET_DEPTH_FUNC value (== GL depth func: 0x200..0x207) */
    unsigned long  DepthMask;       /* 0/1 — write to the depth buffer */
    unsigned long  CullEnable;      /* 0/1 — enable back-face culling (glEnable(GL_CULL_FACE)) */
    /* Texturing (glEnable(GL_TEXTURE_2D) + a bound texture).  The texture is a
     * linear A8R8G8B8 image previously uploaded via IOCTL_VIDEO_NV2A_TEX_UPLOAD. */
    unsigned long  TexEnable;       /* 0/1 — sample TexOffset and modulate by diffuse */
    unsigned long  TexOffset;       /* GPU offset of the texture in the texture heap */
    unsigned long  TexWidth;
    unsigned long  TexHeight;
    unsigned long  TexFilter;       /* NV097_SET_TEXTURE_FILTER value (mag<<24 | min<<16) */
    unsigned long  TexReplace;      /* 1 = GL_REPLACE (output texture, ignore vertex colour); 0 = MODULATE */
    unsigned long  TexEnvMode;      /* glTexEnv: 0=MODULATE, 1=REPLACE, 2=DECAL, 3=BLEND */
    unsigned long  TexEnvColor;     /* packed 0xAARRGGBB GL_TEXTURE_ENV_COLOR (BLEND constant) */
    unsigned long  TexAddress;      /* NV097_SET_TEXTURE_ADDRESS: U | V<<8 | P<<16 (1=wrap,2=mirror,3=clamp-edge) */
    /* Alpha blending (glEnable(GL_BLEND) + glBlendFunc). */
    unsigned long  BlendEnable;     /* 0/1 */
    unsigned long  BlendSrc;        /* NV097_SET_BLEND_FUNC_SFACTOR value (== GL blend enum) */
    unsigned long  BlendDst;        /* NV097_SET_BLEND_FUNC_DFACTOR value (== GL blend enum) */
    unsigned long  BlendEquation;   /* raw GL blend-equation enum; miniport maps to NV2A (0 = FUNC_ADD) */
    unsigned long  BlendColor;      /* packed 0xAARRGGBB constant blend colour (GL_CONSTANT_*) */
    /* Alpha test (glAlphaFunc) — discards fragments by alpha. */
    unsigned long  AlphaTestEnable; /* 0/1 */
    unsigned long  AlphaFunc;       /* NV097_SET_ALPHA_FUNC value (== GL alpha func 0x200..0x207) */
    unsigned long  AlphaRef;        /* 0..255 reference */
    /* Polygon fill mode (glPolygonMode): NV097_SET_*_POLYGON_MODE value. */
    unsigned long  PolygonMode;     /* 0 = use init default (FILL) */
    /* Polygon offset (glPolygonOffset + glEnable(GL_POLYGON_OFFSET_*)). */
    unsigned long  PolyOffsetEnable; /* bit0=fill, bit1=line, bit2=point (0 = disabled) */
    unsigned long  PolyOffsetFactor; /* IEEE float bits: depth-slope factor */
    unsigned long  PolyOffsetUnits;  /* IEEE float bits: constant bias (units) */
    unsigned long  PointSize;       /* NV097_SET_POINT_SIZE fixed-point (size*8); 0 = default */
    /* Colour write mask (glColorMask): NV097_SET_COLOR_MASK packed
     * (alpha<<24 | red<<16 | green<<8 | blue, each byte 0/1).  0x01010101 = all on. */
    unsigned long  ColorMask;
    /* Stencil (glStencil*; only programmed when the zeta is Z24S8). */
    unsigned long  StencilEnable;   /* 0/1 — glEnable(GL_STENCIL_TEST) */
    unsigned long  StencilFunc;     /* GL/NV stencil func 0x200..0x207 */
    unsigned long  StencilRef;      /* 0..255 reference */
    unsigned long  StencilFuncMask; /* compare mask */
    unsigned long  StencilWriteMask;/* glStencilMask write mask */
    unsigned long  StencilOpFail;   /* GL/NV stencil op (KEEP/ZERO/REPLACE/INCR/DECR/INVERT) */
    unsigned long  StencilOpZFail;
    unsigned long  StencilOpZPass;
    unsigned long  ClearStencil;    /* 0..255 stencil clear value (FLAG_CLEAR_STENCIL) */
    unsigned long  DstX;
    unsigned long  DstY;
    unsigned long  DstW;
    unsigned long  DstH;
    NV2A_3D_VERTEX Verts[NV2A_3D_MAX_VERTS];
} NV2A_DRAW_3D, *PNV2A_DRAW_3D;

/* ------------------------------------------------------------------------- */
/* Kernel-only prototypes -------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* The miniport pulls xboxvmp.h before this header, which defines
 * PXBOXVMP_DEVICE_EXTENSION.  External consumers don't get these decls. */

#if defined(_XBOXVMP_DEVICE_EXTENSION_DEFINED) || defined(XBOXVMP_KERNEL)

struct _XBOXVMP_DEVICE_EXTENSION;

VP_STATUS Nv2aAccelInitialize(struct _XBOXVMP_DEVICE_EXTENSION *Dx);
VOID      Nv2aAccelShutdown(struct _XBOXVMP_DEVICE_EXTENSION *Dx);
BOOLEAN   Nv2aAccelFillRect(struct _XBOXVMP_DEVICE_EXTENSION *Dx,
                            ULONG X, ULONG Y, ULONG W, ULONG H, ULONG Color);
BOOLEAN   Nv2aAccelScreenBlt(struct _XBOXVMP_DEVICE_EXTENSION *Dx,
                             ULONG SrcX, ULONG SrcY, ULONG DstX, ULONG DstY,
                             ULONG W, ULONG H);
BOOLEAN   Nv2aAccelBltEx(struct _XBOXVMP_DEVICE_EXTENSION *Dx,
                         ULONG SrcOffset, ULONG SrcPitch, ULONG SrcX, ULONG SrcY,
                         ULONG DstOffset, ULONG DstPitch, ULONG DstX, ULONG DstY,
                         ULONG W, ULONG H);
VOID      Nv2aAccelWaitIdle(struct _XBOXVMP_DEVICE_EXTENSION *Dx);

/* NV2A 3D (Kelvin) path — nv2a_3d.c */
BOOLEAN   Nv2a3dInitialize(struct _XBOXVMP_DEVICE_EXTENSION *Dx);
BOOLEAN   Nv2a3dDrawTriangles(struct _XBOXVMP_DEVICE_EXTENSION *Dx,
                              const struct _NV2A_DRAW_3D *Draw);
/* Upload a linear A8R8G8B8 texture; returns the assigned GPU offset (0 on failure). */
ULONG     Nv2aTexUpload(struct _XBOXVMP_DEVICE_EXTENSION *Dx,
                        ULONG Width, ULONG Height, ULONG ExistingOffset, const ULONG *Pixels);
/* Read/write a rectangle of the offscreen colour surface (CPU access to VRAM). */
BOOLEAN   Nv2aReadback(struct _XBOXVMP_DEVICE_EXTENSION *Dx,
                       ULONG X, ULONG Y, ULONG Width, ULONG Height, ULONG *Out);
BOOLEAN   Nv2aWriteback(struct _XBOXVMP_DEVICE_EXTENSION *Dx,
                        ULONG X, ULONG Y, ULONG Width, ULONG Height, ULONG Flags, const ULONG *Pixels);

#endif /* kernel guard */

#ifdef __cplusplus
}
#endif

#endif /* _XBOX_NV2A_ACCEL_H_ */
