/* -*- mode: c; c-basic-offset: 3 -*-
 *
 * Copyright 2000 VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/tdfx_context.h,v 1.5 2002/02/24 21:51:10 dawes Exp $ */

/*
 * New fixes:
 *	Daniel Borca <dborca@users.sourceforge.net>, 19 Jul 2004
 *
 * Original rewrite:
 *	Gareth Hughes <gareth@valinux.com>, 29 Sep - 1 Oct 2000
 *
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *
 */

#ifndef __TDFX_CONTEXT_H__
#define __TDFX_CONTEXT_H__

#include <sys/time.h>
#include "dri_util.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#else
#include "glheader.h"
#endif
#if defined(__linux__)
#include <signal.h>
#endif

#include "drm.h"
#include "drm_sarea.h"
#include "tdfx_glide.h"
#include "xmlconfig.h"

#include "clip.h"
#include "context.h"
#include "macros.h"
#include "matrix.h"
#include "imports.h"
#include "mtypes.h"

#include "tdfx_screen.h"




#define TDFX_TMU0		GR_TMU0
#define TDFX_TMU1		GR_TMU1
#define TDFX_TMU_SPLIT		98
#define TDFX_TMU_BOTH		99
#define TDFX_TMU_NONE		100



/* Flags for fxMesa->new_state
 */
#define TDFX_NEW_COLOR		0x0001
#define TDFX_NEW_ALPHA		0x0002
#define TDFX_NEW_DEPTH		0x0004
#define TDFX_NEW_FOG		0x0008
#define TDFX_NEW_STENCIL	0x0010
#define TDFX_NEW_CLIP		0x0020
#define TDFX_NEW_VIEWPORT	0x0040
#define TDFX_NEW_CULL		0x0080
#define TDFX_NEW_GLIDE		0x0100
#define TDFX_NEW_TEXTURE	0x0200
#define TDFX_NEW_CONTEXT	0x0400
#define TDFX_NEW_LINE		0x0800
#define TDFX_NEW_RENDER         0x1000
#define TDFX_NEW_STIPPLE	0x2000
#define TDFX_NEW_TEXTURE_BIND   0x4000 /* experimental */


/* Flags for fxMesa->dirty
 */
#define TDFX_UPLOAD_COLOR_COMBINE	0x00000001
#define TDFX_UPLOAD_ALPHA_COMBINE	0x00000002
#define TDFX_UPLOAD_RENDER_BUFFER	0x00000004
#define TDFX_UPLOAD_ALPHA_TEST		0x00000008
#define TDFX_UPLOAD_ALPHA_REF		0x00000010
#define TDFX_UPLOAD_BLEND_FUNC		0x00000020
#define TDFX_UPLOAD_DEPTH_MODE		0x00000040
#define TDFX_UPLOAD_DEPTH_BIAS		0x00000080
#define TDFX_UPLOAD_DEPTH_FUNC		0x00000100
#define TDFX_UPLOAD_DEPTH_MASK		0x00000200
#define TDFX_UPLOAD_FOG_MODE		0x00000400
#define TDFX_UPLOAD_FOG_COLOR		0x00000800
#define TDFX_UPLOAD_FOG_TABLE		0x00001000
#define TDFX_UPLOAD_CONSTANT_COLOR	0x00002000

#define TDFX_UPLOAD_CLIP		0x00002000
#define TDFX_UPLOAD_CULL		0x00004000
#define TDFX_UPLOAD_VERTEX_LAYOUT	0x00008000
#define TDFX_UPLOAD_COLOR_MASK		0x00010000
#define TDFX_UPLOAD_DITHER		0x00040000
#define TDFX_UPLOAD_STENCIL		0x00080000

#define TDFX_UPLOAD_TEXTURE_SOURCE	0x00100000
#define TDFX_UPLOAD_TEXTURE_PARAMS	0x00200000
#define TDFX_UPLOAD_TEXTURE_PALETTE	0x00400000
#define TDFX_UPLOAD_TEXTURE_ENV		0x00800000
#define TDFX_UPLOAD_TEXTURE_IMAGES	0x01000000

#define TDFX_UPLOAD_LINE		0x02000000

#define TDFX_UPLOAD_STIPPLE		0x04000000

/* Flags for software fallback cases */
/* See correponding strings in tdfx_tris.c */
#define TDFX_FALLBACK_TEXTURE_MAP	0x0001
#define TDFX_FALLBACK_DRAW_BUFFER	0x0002
#define TDFX_FALLBACK_SPECULAR		0x0004
#define TDFX_FALLBACK_STENCIL		0x0008
#define TDFX_FALLBACK_RENDER_MODE	0x0010
#define TDFX_FALLBACK_LOGICOP		0x0020
#define TDFX_FALLBACK_TEXTURE_ENV	0x0040
#define TDFX_FALLBACK_TEXTURE_BORDER	0x0080
#define TDFX_FALLBACK_COLORMASK		0x0100
#define TDFX_FALLBACK_BLEND		0x0200
#define TDFX_FALLBACK_LINE_STIPPLE	0x0400
#define TDFX_FALLBACK_DISABLE		0x0800

/* Different Glide vertex layouts
 */
#define TDFX_LAYOUT_TINY	0
#define TDFX_LAYOUT_NOTEX	1
#define TDFX_LAYOUT_SINGLE	2
#define TDFX_LAYOUT_MULTI	3
#define TDFX_LAYOUT_PROJ1	4
#define TDFX_LAYOUT_PROJ2	5
#define TDFX_NUM_LAYOUTS	6

#define TDFX_XY_OFFSET		0
#define TDFX_Z_OFFSET		8
#define TDFX_Q_OFFSET		12
#define TDFX_ARGB_OFFSET	16
#define TDFX_FOG_OFFSET         20
#define TDFX_ST0_OFFSET		24
#define TDFX_ST1_OFFSET		32
#define TDFX_Q0_OFFSET		40
#define TDFX_Q1_OFFSET		44


/* Flags for buffer clears
 */
#define TDFX_FRONT		0x1
#define TDFX_BACK		0x2
#define TDFX_DEPTH		0x4
#define TDFX_STENCIL		0x8

/*
 * Subpixel offsets to adjust Mesa's (true) window coordinates to
 * Glide coordinates.  We need these to ensure precise rasterization.
 * Otherwise, we'll fail a bunch of conformance tests.
 */
#define TRI_X_OFFSET    ( 0.0F)
#define TRI_Y_OFFSET    ( 0.0F)
#define LINE_X_OFFSET   ( 0.0F)
#define LINE_Y_OFFSET   ( 0.125F)
#define PNT_X_OFFSET    ( 0.375F)
#define PNT_Y_OFFSET    ( 0.375F)


#define TDFX_DEPTH_BIAS_SCALE	128

/* Including xf86PciInfo.h causes a bunch of errors
 */
#ifndef PCI_CHIP_BANSHEE
#define PCI_CHIP_BANSHEE	0x0003
#define PCI_CHIP_VOODOO3	0x0005
#define PCI_CHIP_VOODOO4	0x0009
#define PCI_CHIP_VOODOO5	0x0009
#endif

#define TDFX_IS_BANSHEE( fxMesa ) \
		( fxMesa->fxScreen->deviceID == PCI_CHIP_BANSHEE )
#define TDFX_IS_VOODOO3( fxMesa ) \
		( fxMesa->fxScreen->deviceID == PCI_CHIP_VOODOO3 )
#define TDFX_IS_VOODOO4( fxMesa ) \
		( fxMesa->fxScreen->deviceID == PCI_CHIP_VOODOO4 )
#define TDFX_IS_VOODOO5( fxMesa ) \
		( fxMesa->fxScreen->deviceID == PCI_CHIP_VOODOO5 )
#define TDFX_IS_NAPALM( fxMesa ) \
                ( (fxMesa->fxScreen->deviceID == PCI_CHIP_VOODOO4) || \
                  (fxMesa->fxScreen->deviceID == PCI_CHIP_VOODOO5) )


#define PACK_BGRA32(R, G, B, A)  \
    ( (((GLuint) (R)) << 16) | \
      (((GLuint) (G)) <<  8) | \
      (((GLuint) (B))      ) | \
      (((GLuint) (A)) << 24) )

#define PACK_RGBA32(R, G, B, A)  \
    ( (((GLuint) (R))      ) | \
      (((GLuint) (G)) <<  8) | \
      (((GLuint) (B)) << 16) | \
      (((GLuint) (A)) << 24) )

/*
 * The first two macros are to pack 8 bit color
 * channel values into a 565 format.
 */
#define PACK_RGB16(R, G, B)         \
    ((((GLuint) (R) & 0xF8) << 8) | \
     (((GLuint) (G) & 0xFC) << 3) | \
      (((GLuint) (B) & 0xFF)         >> 3))
#define PACK_BGR16(R, G, B)         \
    ((((GLuint) (B) & 0xF8) << 8) | \
     (((GLuint) (G) & 0xFC) << 3) | \
     (((GLuint) (R) & 0xFF) >> 3))
/*
 * The second two macros pack 8 bit color channel values
 * into 1555 values.
 */
#define PACK_RGBA16(R, G, B, A)       \
    (((((GLuint) (A) & 0xFF) > 0) << 15)| \
     (((GLuint) (R)  & 0xF8)      << 7) | \
     (((GLuint) (G)  & 0xF8)      << 2) | \
     (((GLuint) (B)  & 0xF8)      >> 3))
#define PACK_BGRA16(R, G, B, A) \
    (((((GLuint) (A) & 0xFF) > 0) << 15)| \
      (((GLuint) (B) & 0xF8)     << 7)  | \
      (((GLuint) (G) & 0xF8)     << 2)  | \
      (((GLuint) (R) & 0xF8)     >> 3))

/* Used in calls to grColorMaskv()...
 */
extern const GLboolean false4[4];
extern const GLboolean true4[4];


typedef struct tdfx_context tdfxContextRec;
typedef struct tdfx_context *tdfxContextPtr;


typedef struct {
   volatile int fifoPtr;
   volatile int fifoRead;
   volatile int fifoOwner;
   volatile int ctxOwner;
   volatile int texOwner;
}
TDFXSAREAPriv;


typedef struct {
   GLuint swapBuffer;
   GLuint reqTexUpload;
   GLuint texUpload;
   GLuint memTexUpload;
   GLuint texSwaps;
} tdfxStats;



/*
 *  Memory range from startAddr to endAddr-1
 */
typedef struct mem_range {
   struct mem_range *next;
   FxU32 startAddr, endAddr;
}
tdfxMemRange;


typedef struct {
    GLsizei		width, height;	/* image size */
    GLint		wScale, hScale; /* scale factors */
    GrTextureFormat_t	glideFormat;	/* Glide image format */
}
tdfxMipMapLevel;


#define TDFX_NUM_TMU		2


typedef struct tdfxTexInfo_t
{
   GLboolean isInTM;
   GLboolean reloadImages;  /* if true, resend images to Glide */
   GLuint lastTimeUsed;
   FxU32 whichTMU;

   GrTexInfo info;
   GrAspectRatio_t aspectRatio;
   tdfxMemRange *tm[TDFX_NUM_TMU];

   GLint minLevel, maxLevel;
   GrTextureFilterMode_t minFilt;
   GrTextureFilterMode_t magFilt;
   GrTextureClampMode_t sClamp;
   GrTextureClampMode_t tClamp;
   FxBool LODblend;
   GrMipMapMode_t mmMode;

   GLfloat sScale, tScale;  /* texcoord scale factor */

   GrTexTable_t paltype;
   GuTexPalette palette;

   GLboolean padded;
}
tdfxTexInfo;


#define TDFX_TEXTURE_DATA(mesaObj) ((tdfxTexInfo *)((mesaObj)->DriverData))

#define TDFX_TEXIMAGE_DATA(mesaImg) ((tdfxMipMapLevel *)((mesaImg)->DriverData))



/*
 * This is state which may be shared by several tdfx contexts.
 * It hangs off of Mesa's gl_shared_state object (ctx->Shared->DriverData).
 */
struct tdfxSharedState {
   GLboolean umaTexMemory;
   GLuint totalTexMem[TDFX_NUM_TMU]; /* constant */
   GLuint freeTexMem[TDFX_NUM_TMU]; /* changes as we go */
   tdfxMemRange *tmPool;
   tdfxMemRange *tmFree[TDFX_NUM_TMU];
};



/* ================================================================
 * The vertex structures.
 */
/* The size of this union is not of relevence:
 */
typedef struct tdfx_vertex_t {
   GLfloat x, y, z;			/* Coordinates in screen space */
   GLfloat rhw;				/* Reciprocal homogeneous w */
   GLubyte color[4];		/* Diffuse color */
   GLfloat fog;
   GLfloat tu0, tv0;		/* Texture 0 coordinates */
   GLfloat tu1, tv1;		/* Texture 1 coordinates */
   GLfloat tq0, tq1;		/* Texture 0/1 q coords */
   unsigned char pspec[4];	/* B, G, R, A [0..255] */
   float psize;		/* point size */
   long pad[16 - 14];	/* ensure 64b structure */
} tdfxVertex, *tdfxVertexPtr;


/* ================================================================
 *
 * We want to keep a mirror of the Glide function call parameters so we
 * can avoid updating our state too often.
 *
 * Each of these broad groups will typically have a new state flag
 * associated with it, and will be updated together.  The individual
 * Glide function calls each have a dirty flag and will only be called
 * when absolutely necessary.
 */

/* for grTexSource() */
struct tdfx_texsource {
   FxU32 StartAddress;
   FxU32 EvenOdd;
   GrTexInfo *Info;
};

/* Texture object params */
struct tdfx_texparams {
   GrTextureClampMode_t sClamp;
   GrTextureClampMode_t tClamp;
   GrTextureFilterMode_t minFilt;
   GrTextureFilterMode_t magFilt;
   GrMipMapMode_t mmMode;
   FxBool LODblend;
   GLfloat LodBias;
};

/* for grTexDownloadTable() texture palettes */
struct tdfx_texpalette {
   GrTexTable_t Type;
   void *Data;
};

/* for Voodoo3/Banshee's grColorCombine() and grAlphaCombine() */
struct tdfx_combine {
   GrCombineFunction_t Function;	/* Combine function */
   GrCombineFactor_t Factor;		/* Combine scale factor */
   GrCombineLocal_t Local;		/* Local combine source */
   GrCombineOther_t Other;		/* Other combine source */
   FxBool Invert;			/* Combine result inversion flag */
};

/* for Voodoo3's grTexCombine() */
struct tdfx_texcombine {
   GrCombineFunction_t FunctionRGB;
   GrCombineFactor_t FactorRGB;
   GrCombineFunction_t FunctionAlpha;
   GrCombineFactor_t FactorAlpha;
   FxBool InvertRGB;
   FxBool InvertAlpha;
};


/* for Voodoo5's grColorCombineExt() */
struct tdfx_combine_color_ext {
   GrCCUColor_t SourceA;
   GrCombineMode_t ModeA;
   GrCCUColor_t SourceB;
   GrCombineMode_t ModeB;
   GrCCUColor_t SourceC;
   FxBool InvertC;
   GrCCUColor_t SourceD;
   FxBool InvertD;
   FxU32 Shift;
   FxBool Invert;
};

/* for Voodoo5's grAlphaCombineExt() */
struct tdfx_combine_alpha_ext {
   GrACUColor_t SourceA;
   GrCombineMode_t ModeA;
   GrACUColor_t SourceB;
   GrCombineMode_t ModeB;
   GrACUColor_t SourceC;
   FxBool InvertC;
   GrACUColor_t SourceD;
   FxBool InvertD;
   FxU32 Shift;
   FxBool Invert;
};

/* for Voodoo5's grTexColorCombineExt() */
struct tdfx_color_texenv {
   GrTCCUColor_t SourceA;
   GrCombineMode_t ModeA;
   GrTCCUColor_t SourceB;
   GrCombineMode_t ModeB;
   GrTCCUColor_t SourceC;
   FxBool InvertC;
   GrTCCUColor_t SourceD;
   FxBool InvertD;
   FxU32 Shift;
   FxBool Invert;
};

/* for Voodoo5's grTexAlphaCombineExt() */
struct tdfx_alpha_texenv {
   GrTACUColor_t SourceA;
   GrCombineMode_t ModeA;
   GrTACUColor_t SourceB;
   GrCombineMode_t ModeB;
   GrTACUColor_t SourceC;
   FxBool InvertC;
   GrTCCUColor_t SourceD;
   FxBool InvertD;
   FxU32 Shift;
   FxBool Invert;
};

/* Voodoo5's texture combine environment */
struct tdfx_texcombine_ext {
   struct tdfx_alpha_texenv Alpha;
   struct tdfx_color_texenv Color;
   GrColor_t EnvColor;
};

/* Used to track changes between Glide's state and Mesa's */
struct tdfx_texstate {
   GLuint Enabled[2];              /* values ala ctx->Texture.Unit[i]._ReallyEnabled */
   GLenum EnvMode[TDFX_NUM_TMU];   /* index is Glide index, not OpenGL */
   GLenum TexFormat[TDFX_NUM_TMU]; /* index is Glide index, not OpenGL */
};

struct tdfx_color {
   GrColor_t ClearColor;		/* Buffer clear color value */
   GrAlpha_t ClearAlpha;		/* Buffer clear alpha value */
   FxBool ColorMask[4];			/* Per-channel write enable flags */

   GrColor_t MonoColor;			/* Constant color value */

   /* Alpha testing */
   GrCmpFnc_t AlphaFunc;		/* Alpha test function */
   GrAlpha_t AlphaRef;			/* Alpha ref value in range [0,255] */

   /* Blending */
   GrAlphaBlendFnc_t BlendSrcRGB;	/* Blend source RGB factor */
   GrAlphaBlendFnc_t BlendDstRGB;	/* Blend destination RGB factor */
   GrAlphaBlendOp_t BlendEqRGB;		/* Blend source RGB op */
   GrAlphaBlendFnc_t BlendSrcA;		/* Blend source alpha factor */
   GrAlphaBlendFnc_t BlendDstA;		/* Blend destination alpha factor */
   GrAlphaBlendOp_t BlendEqA;		/* Blend source alpha op */

   GrDitherMode_t Dither;		/* Dither enable */
};

struct tdfx_depth {
   GrDepthBufferMode_t Mode;		/* Fixed-point Z or floating-point W */
   FxI32 Bias;				/* Polygon offset factor */
   GrCmpFnc_t Func;			/* Depth test function */
   FxU32 Clear;				/* Buffer clear value */
   FxBool Mask;				/* Write enable flag */
};

struct tdfx_stipple {
   GrStippleMode_t Mode;		/* Stipple enable/disable */
   FxU32 Pattern;			/* 8x4 Stipple Pattern */
};

struct tdfx_fog {
   GrFogMode_t Mode;			/* Glide fog mode */
   GrColor_t Color;			/* Fog color value */
   GLenum TableMode;			/* GL fog mode currently in table */
   GrFog_t *Table;			/* Fog value table */
   FxFloat Density;			/* Density >= 0 */
   FxFloat Near;			/* Start distance in eye coords */
   FxFloat Far;				/* End distance in eye coords */
};

struct tdfx_stencil {
   GrCmpFnc_t Function;			/* Stencil function */
   GrStencil_t RefValue;		/* Stencil reference value */
   GrStencil_t ValueMask;		/* Value mask */
   GrStencil_t WriteMask;		/* Write mask */
   GrStencil_t FailFunc;		/* Stencil fail function */
   GrStencil_t ZFailFunc;		/* Stencil pass, depth fail function */
   GrStencil_t ZPassFunc;		/* Stencil pass, depth pass function */
   GrStencil_t Clear;			/* Buffer clear value */
};

struct tdfx_scissor {
   FxU32 minX, minY;			/* Lower left corner */
   FxU32 maxX, maxY;			/* Upper right corner */
};

struct tdfx_viewport {
   GrCoordinateSpaceMode_t Mode;	/* Coordinate space */
   FxI32 X, Y;				/* Position */
   FxI32 Width, Height;			/* Size */
   FxFloat Near, Far;			/* Depth buffer range */
};

struct tdfx_glide {
   void *State;				/* Mirror of internal Glide state */
   GrContext_t Context;			/* Glide context identifier */
   FxI32 Board;				/* Current graphics subsystem */
   GrColorFormat_t ColorFormat;		/* Framebuffer format */
   GrOriginLocation_t Origin;		/* Location of screen space origin */

   FxBool Initialized;			/* Glide initialization done? */

   FxI32 SwapInterval;			/* SwapBuffers interval */
   FxI32 MaxPendingSwaps;		/* Maximum outstanding SwapBuffers */
   FxI32 TextureAlign;

   /* Extensions */
   FxBool HaveCombineExt;		/* COMBINE */
   FxBool HaveCommandTransportExt;	/* COMMAND_TRANSPORT */
   FxBool HaveFogCoordExt;		/* FOGCOORD */
   FxBool HavePixelExt;			/* PIXEXT */
   FxBool HaveTextureBufferExt;		/* TEXTUREBUFFER */
   FxBool HaveTexFmtExt;		/* TEXFMT */
   FxBool HaveTexUMAExt;		/* TEXUMA */
   FxBool HaveMirrorExt;		/* MIRROR */
   FxBool HaveTexus2;			/* Texus 2 - FXT1 */

   /* Glide library function pointers */
   void (*grDrawPoint)( const void *pt );
   void (*grDrawLine)( const void *v1, const void *v2 );
   void (*grDrawTriangle)( const void *a, const void *b, const void *c );
   void (*grVertexLayout)(FxU32 param, FxI32 offset, FxU32 mode);
   void (*grDrawVertexArray)(FxU32 mode, FxU32 Count, void *pointers);
   void (*grDrawVertexArrayContiguous)(FxU32 mode, FxU32 Count,
                                       void *pointers, FxU32 stride);
   void (*grBufferClear)( GrColor_t color, GrAlpha_t alpha, FxU32 depth );
   void (*grBufferSwap)( FxU32 swap_interval );
   void (*grRenderBuffer)( GrBuffer_t buffer );
   void (*grErrorSetCallback)( GrErrorCallbackFnc_t fnc );
   void (*grFinish)(void);
   void (*grFlush)(void);
   GrContext_t (*grSstWinOpen)(FxU32                hWnd,
                               GrScreenResolution_t screen_resolution,
                               GrScreenRefresh_t    refresh_rate,
                               GrColorFormat_t      color_format,
                               GrOriginLocation_t   origin_location,
                               int                  nColBuffers,
                               int                  nAuxBuffers);
   void (*grSstWinClose)( GrContext_t context );
/* Not used */
#if 0
   void (*grSetNumPendingBuffers)(FxI32 NumPendingBuffers);
#endif
   void (*grSelectContext)( GrContext_t context );
   void (*grSstOrigin)(GrOriginLocation_t  origin);
   void (*grSstSelect)( int which_sst );
   void (*grAlphaBlendFunction)(GrAlphaBlendFnc_t rgb_sf,
                                GrAlphaBlendFnc_t rgb_df,
                                GrAlphaBlendFnc_t alpha_sf,
                                GrAlphaBlendFnc_t alpha_df);
   void (*grAlphaCombine)(GrCombineFunction_t function,
                          GrCombineFactor_t factor,
                          GrCombineLocal_t local, GrCombineOther_t other,
                          FxBool invert);
   void (*grAlphaControlsITRGBLighting)( FxBool enable );
   void (*grAlphaTestFunction)( GrCmpFnc_t function );
   void (*grAlphaTestReferenceValue)( GrAlpha_t value );
   void (*grChromakeyMode)( GrChromakeyMode_t mode );
   void (*grChromakeyValue)( GrColor_t value );
   void (*grClipWindow)( FxU32 minx, FxU32 miny, FxU32 maxx, FxU32 maxy );
   void (*grColorCombine)( GrCombineFunction_t function,
                           GrCombineFactor_t factor,
                           GrCombineLocal_t local,
                           GrCombineOther_t other,
                           FxBool invert );
   void (*grColorMask)( FxBool rgb, FxBool a );
   void (*grCullMode)( GrCullMode_t mode );
   void (*grConstantColorValue)( GrColor_t value );
   void (*grDepthBiasLevel)( FxI32 level );
   void (*grDepthBufferFunction)( GrCmpFnc_t function );
   void (*grDepthBufferMode)( GrDepthBufferMode_t mode );
   void (*grDepthMask)( FxBool mask );
   void (*grDisableAllEffects)( void );
   void (*grDitherMode)( GrDitherMode_t mode );
   void (*grFogColorValue)( GrColor_t fogcolor );
   void (*grFogMode)( GrFogMode_t mode );
   void (*grFogTable)( const GrFog_t ft[] );
   void (*grLoadGammaTable)( FxU32 nentries, FxU32 *red, FxU32 *green, FxU32 *blue);
   void (*grSplash)(float x, float y, float width, float height, FxU32 frame);
   FxU32 (*grGet)( FxU32 pname, FxU32 plength, FxI32 *params );
   const char * (*grGetString)( FxU32 pname );
   FxI32 (*grQueryResolutions)( const GrResolution *resTemplate,
                                GrResolution *output );
   FxBool (*grReset)( FxU32 what );
   GrProc (*grGetProcAddress)( char *procName );
   void (*grEnable)( GrEnableMode_t mode );
   void (*grDisable)( GrEnableMode_t mode );
   void (*grCoordinateSpace)( GrCoordinateSpaceMode_t mode );
   void (*grDepthRange)( FxFloat n, FxFloat f );
   void (*grStippleMode)( GrStippleMode_t mode );
   void (*grStipplePattern)( GrStipplePattern_t mode );
   void (*grViewport)( FxI32 x, FxI32 y, FxI32 width, FxI32 height );
   FxU32 (*grTexCalcMemRequired)(GrLOD_t lodmin, GrLOD_t lodmax,
                                GrAspectRatio_t aspect, GrTextureFormat_t fmt);
   FxU32 (*grTexTextureMemRequired)( FxU32 evenOdd, GrTexInfo *info );
   FxU32 (*grTexMinAddress)( GrChipID_t tmu );
   FxU32 (*grTexMaxAddress)( GrChipID_t tmu );
   void (*grTexNCCTable)( GrNCCTable_t table );
   void (*grTexSource)( GrChipID_t tmu, FxU32 startAddress,
                        FxU32 evenOdd, GrTexInfo *info );
   void (*grTexClampMode)( GrChipID_t tmu,
                           GrTextureClampMode_t s_clampmode,
                           GrTextureClampMode_t t_clampmode );
   void (*grTexCombine)( GrChipID_t tmu,
                         GrCombineFunction_t rgb_function,
                         GrCombineFactor_t rgb_factor, 
                         GrCombineFunction_t alpha_function,
                         GrCombineFactor_t alpha_factor,
                         FxBool rgb_invert,
                         FxBool alpha_invert);
   void (*grTexDetailControl)( GrChipID_t tmu, int lod_bias,
                               FxU8 detail_scale, float detail_max );
   void (*grTexFilterMode)( GrChipID_t tmu,
                            GrTextureFilterMode_t minfilter_mode,
                            GrTextureFilterMode_t magfilter_mode );
   void (*grTexLodBiasValue)(GrChipID_t tmu, float bias );
   void (*grTexDownloadMipMap)( GrChipID_t tmu, FxU32 startAddress,
                                FxU32 evenOdd, GrTexInfo *info );
   void (*grTexDownloadMipMapLevel)( GrChipID_t        tmu,
                                     FxU32             startAddress,
                                     GrLOD_t           thisLod,
                                     GrLOD_t           largeLod,
                                     GrAspectRatio_t   aspectRatio,
                                     GrTextureFormat_t format,
                                     FxU32             evenOdd,
                                     void              *data );
   FxBool (*grTexDownloadMipMapLevelPartial)( GrChipID_t        tmu,
                                              FxU32             startAddress,
                                              GrLOD_t           thisLod,
                                              GrLOD_t           largeLod,
                                              GrAspectRatio_t   aspectRatio,
                                              GrTextureFormat_t format,
                                              FxU32             evenOdd,
                                              void              *data,
                                              int               start,
                                              int               end );
   void (*grTexDownloadTable)( GrTexTable_t type, void *data );
   void (*grTexDownloadTablePartial)( GrTexTable_t type, 
                                      void *data, int start, int end );
   void (*grTexMipMapMode)( GrChipID_t tmu, GrMipMapMode_t mode,
                            FxBool lodBlend );
   void (*grTexMultibase)( GrChipID_t tmu, FxBool enable );
   void (*grTexMultibaseAddress)( GrChipID_t       tmu,
                                  GrTexBaseRange_t range,
                                  FxU32            startAddress,
                                  FxU32            evenOdd,
                                  GrTexInfo        *info );
   FxBool (*grLfbLock)( GrLock_t type, GrBuffer_t buffer,
                        GrLfbWriteMode_t writeMode,
                        GrOriginLocation_t origin, FxBool pixelPipeline, 
                        GrLfbInfo_t *info );
   FxBool (*grLfbUnlock)( GrLock_t type, GrBuffer_t buffer );
   void (*grLfbConstantAlpha)( GrAlpha_t alpha );
   void (*grLfbConstantDepth)( FxU32 depth );
   void (*grLfbWriteColorSwizzle)(FxBool swizzleBytes, FxBool swapWords);
   void (*grLfbWriteColorFormat)(GrColorFormat_t colorFormat);
   FxBool (*grLfbWriteRegion)( GrBuffer_t dst_buffer, 
                               FxU32 dst_x, FxU32 dst_y, 
                               GrLfbSrcFmt_t src_format, 
                               FxU32 src_width, FxU32 src_height, 
                               FxBool pixelPipeline,
                               FxI32 src_stride, void *src_data );
   FxBool (*grLfbReadRegion)( GrBuffer_t src_buffer,
                              FxU32 src_x, FxU32 src_y,
                              FxU32 src_width, FxU32 src_height,
                              FxU32 dst_stride, void *dst_data );
   void (*grGlideInit)( void );
   void (*grGlideShutdown)( void );
   void (*grGlideGetState)( void *state );
   void (*grGlideSetState)( const void *state );
   void (*grGlideGetVertexLayout)( void *layout );
   void (*grGlideSetVertexLayout)( const void *layout );
   /* Glide utility functions */
   void (*guFogGenerateExp)( GrFog_t *fogtable, float density );
   void (*guFogGenerateExp2)( GrFog_t *fogtable, float density );
   void (*guFogGenerateLinear)(GrFog_t *fogtable, float nearZ, float farZ );
   /* DRI functions */
   void (*grDRIOpen)( char *pFB, char *pRegs, int deviceID,
                      int width, int height,
                      int mem, int cpp, int stride,
                      int fifoOffset, int fifoSize,
                      int fbOffset, int backOffset, int depthOffset,
                      int textureOffset, int textureSize,
                      volatile int *fifoPtr, volatile int *fifoRead );
   void (*grDRIPosition)( int x, int y, int w, int h,
                          int numClip, drm_clip_rect_t *pClip );
   void (*grDRILostContext)( void );
   void (*grDRIImportFifo)( int fifoPtr, int fifoRead );
   void (*grDRIInvalidateAll)( void );
   void (*grDRIResetSAREA)( void );
   void (*grDRIBufferSwap)( FxU32 swapInterval );
   /* Glide extensions */
   /* PIXEXT extension */
   void (*grStencilFunc)( GrCmpFnc_t func, GrStencil_t ref, GrStencil_t mask );
   void (*grStencilMask)( GrStencil_t mask );
   void (*grStencilOp)( GrStencilOp_t fail, GrStencilOp_t zfail,
                        GrStencilOp_t zpass );
   void (*grBufferClearExt)( GrColor_t color, GrAlpha_t alpha,
                             FxU32 depth, GrStencil_t stencil );
   void (*grColorMaskExt)( FxBool r, FxBool g, FxBool b, FxBool a );
   /* COMBINE extension */
   void (*grColorCombineExt)( GrCCUColor_t a, GrCombineMode_t a_mode,
                              GrCCUColor_t b, GrCombineMode_t b_mode,
                              GrCCUColor_t c, FxBool c_invert,
                              GrCCUColor_t d, FxBool d_invert,
                              FxU32 shift, FxBool invert );
   void (*grTexColorCombineExt)( FxU32 tmu,
                                 GrTCCUColor_t a, GrCombineMode_t a_mode,
                                 GrTCCUColor_t b, GrCombineMode_t b_mode,
                                 GrTCCUColor_t c, FxBool c_invert,
                                 GrTCCUColor_t d, FxBool d_invert,
                                 FxU32 shift, FxBool invert );
   void (*grAlphaCombineExt)( GrACUColor_t a, GrCombineMode_t a_mode,
                              GrACUColor_t b, GrCombineMode_t b_mode,
                              GrACUColor_t c, FxBool c_invert,
                              GrACUColor_t d, FxBool d_invert,
                              FxU32 shift, FxBool invert );
   void (*grTexAlphaCombineExt)( FxU32 tmu,
                                 GrTACUColor_t a, GrCombineMode_t a_mode,
                                 GrTACUColor_t b, GrCombineMode_t b_mode,
                                 GrTACUColor_t c, FxBool c_invert,
                                 GrTACUColor_t d, FxBool d_invert,
                                 FxU32 shift, FxBool invert );
   void (*grAlphaBlendFunctionExt)( GrAlphaBlendFnc_t rgb_sf,
                                    GrAlphaBlendFnc_t rgb_df,
                                    GrAlphaBlendOp_t rgb_op,
                                    GrAlphaBlendFnc_t alpha_sf,
                                    GrAlphaBlendFnc_t alpha_df,
                                    GrAlphaBlendOp_t alpha_op );
   void (*grConstantColorValueExt)( FxU32 tmu, GrColor_t value );
   /* Texus 2 */
   void (*txImgQuantize)( void *xxx_unknown_arguments );
   void (*txImgDequantizeFXT1)( void *txMip, void *pxMip );
   void (*txErrorSetCallback)( void *fnc );
};

typedef void (*tdfx_tri_func)( tdfxContextPtr, tdfxVertex *, tdfxVertex *,
			       tdfxVertex * );
typedef void (*tdfx_line_func)( tdfxContextPtr, tdfxVertex *, tdfxVertex * );
typedef void (*tdfx_point_func)( tdfxContextPtr, tdfxVertex * );

struct tdfx_context {
   /* Set once and never changed:
    */
   GLcontext *glCtx;			/* The core Mesa context */

   GLuint new_gl_state;
   GLuint new_state;
   GLuint dirty;

   /* Mirror of hardware state, Glide parameters
    */
   GLuint tmu_source[TDFX_NUM_TMU];
   struct tdfx_texsource	TexSource[TDFX_NUM_TMU];
   struct tdfx_texparams	TexParams[TDFX_NUM_TMU];
   struct tdfx_texpalette	TexPalette;

   /* Voodoo3 texture/color combine state */
   struct tdfx_combine		ColorCombine;
   struct tdfx_combine		AlphaCombine;
   struct tdfx_texcombine	TexCombine[TDFX_NUM_TMU];

   /* Voodoo5 texture/color combine state */
   struct tdfx_combine_color_ext	ColorCombineExt;
   struct tdfx_combine_alpha_ext	AlphaCombineExt;
   struct tdfx_texcombine_ext		TexCombineExt[TDFX_NUM_TMU];

   /* Tracks tex state difference between Glide and Mesa */
   struct tdfx_texstate		TexState;

   GrBuffer_t		DrawBuffer;	/* Current draw buffer */
   GrBuffer_t		ReadBuffer;	/* Current read buffer */

   struct tdfx_color	Color;
   struct tdfx_depth	Depth;
   struct tdfx_fog	Fog;
   struct tdfx_stencil	Stencil;
   struct tdfx_scissor	Scissor;
   struct tdfx_viewport	Viewport;
   struct tdfx_stipple	Stipple;

   GrCullMode_t		CullMode;

   struct tdfx_glide	Glide;

   /* Fallback rasterization functions 
    */
   tdfx_point_func draw_point;
   tdfx_line_func draw_line;
   tdfx_tri_func draw_triangle;


   /* Variable-size Glide vertex formats
    */
   GLuint vertexFormat;		/* the current format */
   void *layout[TDFX_NUM_LAYOUTS];
   tdfxVertex *verts;
   
   GLfloat hw_viewport[16];
   
   GLuint SetupIndex;
   GLuint SetupNewInputs;
   GLuint RenderIndex;
   GLuint Fallback;
   GLenum render_primitive;	/* what GL thinks */
   GLenum raster_primitive;	/* what the hardware thinks */

   GLfloat sScale0, tScale0;
   GLfloat sScale1, tScale1;

   GLuint texBindNumber;
   GLint tmuSrc;

   int screen_width;
   int screen_height;

   GLboolean haveTwoTMUs;      /* True if we have 2 tmu's  */
   GLboolean haveHwAlpha;
   GLboolean haveHwStencil;
   GLboolean haveHwStipple;

   GLint maxPendingSwapBuffers;

   char rendererString[100];

   /* stuff added for DRI */
   __DRIscreenPrivate *driScreen;
   __DRIcontextPrivate *driContext;

   /**
    * DRI drawable bound to this context for drawing.
    */
   __DRIdrawablePrivate	*driDrawable;

   /**
    * DRI drawable bound to this context for reading.
    */
   __DRIdrawablePrivate	*driReadable;

   drm_context_t hHWContext;
   drm_hw_lock_t *driHwLock;
   int driFd;
   tdfxScreenPrivate *fxScreen;
   TDFXSAREAPriv *sarea;


   /*
    * Changes during execution:
    */
   int width, height;   /* size of window */
   int x_offset;        /* distance from window left to screen left */
   int y_offset;        /* distance from window top to screen top */
   int y_delta;         /* distance from window bottom to screen bottom */

   int numClipRects;
   drm_clip_rect_t *pClipRects;
   GLboolean scissoredClipRects;  /* if true, pClipRects is private storage */

   GuTexPalette glbPalette;         /* global texture palette */

   tdfxStats stats;

   /* Configuration cache
    */
   driOptionCache optionCache;
};

#define TDFX_CONTEXT(ctx)	((tdfxContextPtr)((ctx)->DriverCtx))


extern GLboolean
tdfxCreateContext( const __GLcontextModes *mesaVis,
                   __DRIcontextPrivate *driContextPriv,
                   void *sharedContextPrivate );

extern void
tdfxDestroyContext( __DRIcontextPrivate *driContextPriv );

extern GLboolean
tdfxUnbindContext( __DRIcontextPrivate *driContextPriv );

extern GLboolean
tdfxMakeCurrent( __DRIcontextPrivate *driContextPriv,
                 __DRIdrawablePrivate *driDrawPriv,
                 __DRIdrawablePrivate *driReadPriv );

extern GLboolean
tdfxInitGlide( tdfxContextPtr tmesa );

extern void
FX_grColorMaskv(GLcontext *ctx, const GLboolean rgba[4]);

extern void
FX_grColorMaskv_NoLock(GLcontext *ctx, const GLboolean rgba[4]);


/* Color packing utilities
 */
#define TDFXPACKCOLOR332( r, g, b )					   \
   (((b) & 0xe0) | (((g) & 0xe0) >> 3) | (((r) & 0xc0) >> 6))

#define TDFXPACKCOLOR1555( r, g, b, a )					   \
   ((((r) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((b) & 0xf8) >> 3) |	   \
    ((a) ? 0x8000 : 0))

#define TDFXPACKCOLOR565( r, g, b )					   \
   ((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | (((b) & 0xf8) >> 3))

#define TDFXPACKCOLOR888( r, g, b )					   \
   (((b) << 16) | ((g) << 8) | (r))

#define TDFXPACKCOLOR8888( r, g, b, a )					   \
   (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

#define TDFXPACKCOLOR4444( r, g, b, a )					   \
   ((((a) & 0xf0) << 8) | (((b) & 0xf0) << 4) | ((g) & 0xf0) | ((r) >> 4))

static __inline__ GrColor_t tdfxPackColor( GLuint cpp,
					   GLubyte r, GLubyte g,
					   GLubyte b, GLubyte a )
{
   switch ( cpp ) {
   case 2:
      return TDFXPACKCOLOR565( r, g, b );
   case 4:
      return TDFXPACKCOLOR8888( r, g, b, a );
   default:
      return 0;
  }
}

#define DO_DEBUG		1
#if DO_DEBUG
extern int TDFX_DEBUG;
#else
#define TDFX_DEBUG		0
#endif

#define DEBUG_ALWAYS_SYNC	0x01
#define DEBUG_VERBOSE_API	0x02
#define DEBUG_VERBOSE_DRI	0x04
#define DEBUG_VERBOSE_FALL	0x08

/* conf */
#define FX_COMPRESS_S3TC_AS_FXT1_HACK 1
#define FX_TC_NAPALM 0

#endif /* __TDFX_CONTEXT_H__ */
