/*
 * Mesa 3-D graphics library
 * Version:  4.0
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Authors:
 *    David Bucciarelli
 *    Brian Paul
 *    Daryll Strauss
 *    Keith Whitwell
 *    Daniel Borca
 *    Hiroshi Morii
 */

/* fxsetup.c - 3Dfx VooDoo rendering mode setup functions */


#ifndef FXDRV_H
#define FXDRV_H

/* If you comment out this define, a variable takes its place, letting
 * you turn debugging on/off from the debugger.
 */

#include "glheader.h"


#if defined(__linux__)
#include <signal.h>
#endif

#include "context.h"
#include "imports.h"
#include "macros.h"
#include "matrix.h"
#include "mtypes.h"

#include "GL/fxmesa.h"
#include "fxglidew.h"

#include "math/m_vector.h"


#define COPY_FLOAT(dst, src)    (dst) = (src)

/* Define some shorter names for these things.
 */
#define XCOORD   GR_VERTEX_X_OFFSET
#define YCOORD   GR_VERTEX_Y_OFFSET
#define ZCOORD   GR_VERTEX_OOZ_OFFSET
#define OOWCOORD GR_VERTEX_OOW_OFFSET

#define S0COORD  GR_VERTEX_SOW_TMU0_OFFSET
#define T0COORD  GR_VERTEX_TOW_TMU0_OFFSET
#define S1COORD  GR_VERTEX_SOW_TMU1_OFFSET
#define T1COORD  GR_VERTEX_TOW_TMU1_OFFSET



#ifdef __i386__
#define FXCOLOR4( c )  (* (int *)c)
#else
#define FXCOLOR4( c ) (      \
  ( ((unsigned int)(c[3]))<<24 ) | \
  ( ((unsigned int)(c[2]))<<16 ) | \
  ( ((unsigned int)(c[1]))<<8 )  | \
  (  (unsigned int)(c[0])) )
#endif

#define TDFXPACKCOLOR1555( r, g, b, a )					   \
   ((((r) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((b) & 0xf8) >> 3) |	   \
    ((a) ? 0x8000 : 0))
#define TDFXPACKCOLOR565( r, g, b )					   \
   ((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | (((b) & 0xf8) >> 3))
#define TDFXPACKCOLOR8888( r, g, b, a )					   \
   (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))



/* fastpath flags first
 */
#define SETUP_TMU0 0x1
#define SETUP_TMU1 0x2
#define SETUP_RGBA 0x4
#define SETUP_SNAP 0x8
#define SETUP_XYZW 0x10
#define SETUP_PTEX 0x20
#define SETUP_PSIZ 0x40
#define SETUP_SPEC 0x80
#define SETUP_FOGC 0x100
#define MAX_SETUP  0x200


#define FX_NUM_TMU 2

#define FX_TMU0      GR_TMU0
#define FX_TMU1      GR_TMU1
#define FX_TMU_SPLIT 98
#define FX_TMU_BOTH  99
#define FX_TMU_NONE  100

/* Used for fxMesa->lastUnitsMode */

#define FX_UM_NONE                  0x00000000

#define FX_UM_E0_REPLACE            0x00000001
#define FX_UM_E0_MODULATE           0x00000002
#define FX_UM_E0_DECAL              0x00000004
#define FX_UM_E0_BLEND              0x00000008
#define FX_UM_E0_ADD		    0x00000010

#define FX_UM_E1_REPLACE            0x00000020
#define FX_UM_E1_MODULATE           0x00000040
#define FX_UM_E1_DECAL              0x00000080
#define FX_UM_E1_BLEND              0x00000100
#define FX_UM_E1_ADD		    0x00000200

#define FX_UM_E_ENVMODE             0x000003ff

#define FX_UM_E0_ALPHA              0x00001000
#define FX_UM_E0_LUMINANCE          0x00002000
#define FX_UM_E0_LUMINANCE_ALPHA    0x00004000
#define FX_UM_E0_INTENSITY          0x00008000
#define FX_UM_E0_RGB                0x00010000
#define FX_UM_E0_RGBA               0x00020000

#define FX_UM_E1_ALPHA              0x00040000
#define FX_UM_E1_LUMINANCE          0x00080000
#define FX_UM_E1_LUMINANCE_ALPHA    0x00100000
#define FX_UM_E1_INTENSITY          0x00200000
#define FX_UM_E1_RGB                0x00400000
#define FX_UM_E1_RGBA               0x00800000

#define FX_UM_E_IFMT                0x00fff000

#define FX_UM_COLOR_ITERATED        0x01000000
#define FX_UM_COLOR_CONSTANT        0x02000000
#define FX_UM_ALPHA_ITERATED        0x04000000
#define FX_UM_ALPHA_CONSTANT        0x08000000


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


/*
  Memory range from startAddr to endAddr-1
*/
typedef struct MemRange_t
{
   struct MemRange_t *next;
   FxU32 startAddr, endAddr;
}
MemRange;

typedef struct
{
   GLsizei width, height;	/* image size */
   GLint wScale, hScale;	/* image scale factor */
   GrTextureFormat_t glideFormat;	/* Glide image format */
}
tfxMipMapLevel;

/*
 * TDFX-specific texture object data.  This hangs off of the
 * struct gl_texture_object DriverData pointer.
 */
typedef struct tfxTexInfo_t
{
   struct tfxTexInfo_t *next;
   struct gl_texture_object *tObj;

   GLuint lastTimeUsed;
   FxU32 whichTMU;
   GLboolean isInTM;

   MemRange *tm[FX_NUM_TMU];

   GLint minLevel, maxLevel;
   GLint baseLevelInternalFormat;

   GrTexInfo info;

   GrTextureFilterMode_t minFilt;
   GrTextureFilterMode_t maxFilt;
   FxBool LODblend;

   GrTextureClampMode_t sClamp;
   GrTextureClampMode_t tClamp;

   GrMipMapMode_t mmMode;

   GLfloat sScale, tScale;

   GrTexTable_t paltype;
   GuTexPalette palette;

   GLboolean fixedPalette;
   GLboolean validated;

   GLboolean padded;
}
tfxTexInfo;

typedef struct
{
   GLuint swapBuffer;
   GLuint reqTexUpload;
   GLuint texUpload;
   GLuint memTexUpload;
}
tfxStats;



typedef struct
{
   /* Alpha test */

   GLboolean alphaTestEnabled;
   GrCmpFnc_t alphaTestFunc;
   GLfloat alphaTestRefValue;

   /* Blend function */

   GLboolean blendEnabled;
   GrAlphaBlendFnc_t blendSrcFuncRGB;
   GrAlphaBlendFnc_t blendDstFuncRGB;
   GrAlphaBlendFnc_t blendSrcFuncAlpha;
   GrAlphaBlendFnc_t blendDstFuncAlpha;
   GrAlphaBlendOp_t blendEqRGB;
   GrAlphaBlendOp_t blendEqAlpha;

   /* Depth test */

   GLboolean depthTestEnabled;
   GLboolean depthMask;
   GrCmpFnc_t depthTestFunc;
   FxI32 depthBias;

   /* Stencil */

   GLboolean stencilEnabled;
   GrCmpFnc_t stencilFunction;		/* Stencil function */
   GrStencil_t stencilRefValue;		/* Stencil reference value */
   GrStencil_t stencilValueMask;	/* Value mask */
   GrStencil_t stencilWriteMask;	/* Write mask */
   GrCmpFnc_t stencilFailFunc;		/* Stencil fail function */
   GrCmpFnc_t stencilZFailFunc;		/* Stencil pass, depth fail function */
   GrCmpFnc_t stencilZPassFunc;		/* Stencil pass, depth pass function */
   GrStencil_t stencilClear;		/* Buffer clear value */
}
tfxUnitsState;




/* Flags for fxMesa->new_state
 */
#define FX_NEW_TEXTURING      0x1
#define FX_NEW_BLEND          0x2
#define FX_NEW_ALPHA          0x4
#define FX_NEW_DEPTH          0x8
#define FX_NEW_FOG            0x10
#define FX_NEW_SCISSOR        0x20
#define FX_NEW_COLOR_MASK     0x40
#define FX_NEW_CULL           0x80
#define FX_NEW_STENCIL        0x100


#define FX_CONTEXT(ctx) ((fxMesaContext)((ctx)->DriverCtx))

#define FX_TEXTURE_DATA(texUnit) fxTMGetTexInfo((texUnit)->_Current)

#define fxTMGetTexInfo(o) ((tfxTexInfo*)((o)->DriverData))

#define FX_MIPMAP_DATA(img)  ((tfxMipMapLevel *) (img)->DriverData)

#define BEGIN_BOARD_LOCK()
#define END_BOARD_LOCK()
#define BEGIN_CLIP_LOOP()
#define END_CLIP_LOOP()




/* Covers the state referenced by IsInHardware:
 */
#define _FX_NEW_IS_IN_HARDWARE (_NEW_TEXTURE|		\
			        _NEW_HINT|		\
			        _NEW_STENCIL|		\
			        _NEW_BUFFERS|		\
			        _NEW_COLOR|		\
			        _NEW_LIGHT)

/* Covers the state referenced by fxDDChooseRenderState
 */
#define _FX_NEW_RENDERSTATE (_FX_NEW_IS_IN_HARDWARE |   \
			     _DD_NEW_FLATSHADE |	\
			     _DD_NEW_TRI_LIGHT_TWOSIDE| \
			     _DD_NEW_TRI_OFFSET |	\
			     _DD_NEW_TRI_UNFILLED |	\
			     _DD_NEW_TRI_SMOOTH |	\
			     _DD_NEW_TRI_STIPPLE |	\
			     _DD_NEW_LINE_SMOOTH |	\
			     _DD_NEW_LINE_STIPPLE |	\
			     _DD_NEW_LINE_WIDTH |	\
			     _DD_NEW_POINT_SMOOTH |	\
			     _DD_NEW_POINT_SIZE |	\
			     _NEW_LINE)


/* Covers the state referenced by fxDDChooseSetupFunction.
 */
#define _FX_NEW_SETUP_FUNCTION (_NEW_LIGHT|	\
			        _NEW_FOG|	\
			        _NEW_TEXTURE|	\
			        _NEW_COLOR)	\


/* lookup table for scaling y bit colors up to 8 bits */
extern GLuint FX_rgb_scale_4[16];
extern GLuint FX_rgb_scale_5[32];
extern GLuint FX_rgb_scale_6[64];

typedef void (*fx_tri_func) (fxMesaContext, GrVertex *, GrVertex *, GrVertex *);
typedef void (*fx_line_func) (fxMesaContext, GrVertex *, GrVertex *);
typedef void (*fx_point_func) (fxMesaContext, GrVertex *);

struct tfxMesaContext
{
   GrTexTable_t glbPalType;
   GuTexPalette glbPalette;

   GLcontext *glCtx;		/* the core Mesa context */
   GLvisual *glVis;		/* describes the color buffer */
   GLframebuffer *glBuffer;	/* the ancillary buffers */

   GLint board;			/* the board used for this context */
   GLint width, height;		/* size of color buffer */

   GrBuffer_t currentFB;

   GLboolean bgrOrder;
   GrColor_t color;
   GrColor_t clearC;
   GrAlpha_t clearA;
   GLuint constColor;
   GrCullMode_t cullMode;

   tfxUnitsState unitsState;
   tfxUnitsState restoreUnitsState;	/* saved during multipass */
   GLboolean multipass;			/* true when drawing intermediate pass */

   GLuint new_state;
   GLuint new_gl_state;

   /* Texture Memory Manager Data
    */
   GLuint texBindNumber;
   GLint tmuSrc;
   GLuint lastUnitsMode;
   GLuint freeTexMem[FX_NUM_TMU];
   MemRange *tmPool;
   MemRange *tmFree[FX_NUM_TMU];

   GLenum fogTableMode;
   GLfloat fogDensity;
   GLfloat fogStart, fogEnd;
   GrFog_t *fogTable;
   GLint textureAlign;
   GLint textureMaxLod;

   /* Vertex building and storage:
    */
   GLuint tmu_source[FX_NUM_TMU];
   GLuint SetupIndex;
   GLuint stw_hint_state;	/* for grHints */
   GrVertex *verts;
   GLboolean snapVertices;      /* needed for older Voodoo hardware */

   /* Rasterization:
    */
   GLuint render_index;
   GLuint fallback;
   GLenum render_primitive;
   GLenum raster_primitive;

   /* Current rasterization functions
    */
   fx_point_func draw_point;
   fx_line_func draw_line;
   fx_tri_func draw_tri;


   /* Keep texture scales somewhere handy:
    */
   GLfloat s0scale;
   GLfloat s1scale;
   GLfloat t0scale;
   GLfloat t1scale;

   GLfloat inv_s0scale;
   GLfloat inv_s1scale;
   GLfloat inv_t0scale;
   GLfloat inv_t1scale;

   /* Glide stuff
    */
   tfxStats stats;
   void *state;

   /* Options */

   GLboolean verbose;
   GLboolean haveTwoTMUs;	/* True if we really have 2 tmu's  */
   GLboolean haveHwAlpha;
   GLboolean haveHwStencil;
   GLboolean haveZBuffer;
   GLboolean haveDoubleBuffer;
   GLboolean haveGlobalPaletteTexture;
   GLint swapInterval;
   GLint maxPendingSwapBuffers;

   GrContext_t glideContext;

   int screen_width;
   int screen_height;
   int clipMinX;
   int clipMaxX;
   int clipMinY;
   int clipMaxY;

   int colDepth;
   GLboolean fsaa;

   /* Glide (per card) capabilities. These get mirrored
    * from `glbHWConfig' when creating a new context...
    */
   GrSstType type;
   FxBool HavePalExt;	/* PALETTE6666 */
   FxBool HavePixExt;	/* PIXEXT */
   FxBool HaveTexFmt;	/* TEXFMT */
   FxBool HaveCmbExt;	/* COMBINE */
   FxBool HaveMirExt;	/* TEXMIRROR */
   FxBool HaveTexUma;	/* TEXUMA */
   FxBool HaveTexus2;	/* Texus 2 - FXT1 */
   struct tdfx_glide Glide;
   char rendererString[64];
};


extern void fxSetupFXUnits(GLcontext *);
extern void fxSetupDDPointers(GLcontext *);

/* fxvb.c:
 */
extern void fxAllocVB(GLcontext * ctx);
extern void fxFreeVB(GLcontext * ctx);
extern void fxPrintSetupFlags(char *msg, GLuint flags );
extern void fxCheckTexSizes( GLcontext *ctx );
extern void fxBuildVertices( GLcontext *ctx, GLuint start, GLuint end,
			     GLuint newinputs );
extern void fxChooseVertexState( GLcontext *ctx );






/* fxtrifuncs:
 */
extern void fxDDInitTriFuncs(GLcontext *);
extern void fxDDChooseRenderState(GLcontext * ctx);


extern void fxUpdateDDSpanPointers(GLcontext *);
extern void fxSetupDDSpanPointers(GLcontext *);

extern void fxPrintTextureData(tfxTexInfo * ti);

extern const struct gl_texture_format *
fxDDChooseTextureFormat( GLcontext *ctx, GLint internalFormat,
                         GLenum srcFormat, GLenum srcType );
extern void fxDDTexImage2D(GLcontext * ctx, GLenum target, GLint level,
			   GLint internalFormat, GLint width, GLint height,
			   GLint border, GLenum format, GLenum type,
			   const GLvoid * pixels,
			   const struct gl_pixelstore_attrib *packing,
			   struct gl_texture_object *texObj,
			   struct gl_texture_image *texImage);
extern void fxDDTexSubImage2D(GLcontext * ctx, GLenum target, GLint level,
			      GLint xoffset, GLint yoffset,
			      GLsizei width, GLsizei height,
			      GLenum format, GLenum type,
			      const GLvoid * pixels,
			      const struct gl_pixelstore_attrib *packing,
			      struct gl_texture_object *texObj,
			      struct gl_texture_image *texImage);
extern void fxDDCompressedTexImage2D(GLcontext *ctx, GLenum target,
                                     GLint level, GLint internalFormat,
                                     GLsizei width, GLsizei height, GLint border,
                                     GLsizei imageSize, const GLvoid *data,
                                     struct gl_texture_object *texObj,
                                     struct gl_texture_image *texImage);
extern void fxDDCompressedTexSubImage2D(GLcontext *ctx, GLenum target,
                                        GLint level, GLint xoffset,
                                        GLint yoffset, GLsizei width,
                                        GLint height, GLenum format,
                                        GLsizei imageSize, const GLvoid *data,
                                        struct gl_texture_object *texObj,
                                        struct gl_texture_image *texImage);
extern void fxDDTexImage1D(GLcontext * ctx, GLenum target, GLint level,
			   GLint internalFormat, GLint width,
			   GLint border, GLenum format, GLenum type,
			   const GLvoid * pixels,
			   const struct gl_pixelstore_attrib *packing,
			   struct gl_texture_object *texObj,
			   struct gl_texture_image *texImage);
extern void fxDDTexSubImage1D(GLcontext * ctx, GLenum target, GLint level,
			      GLint xoffset, GLint width,
			      GLenum format, GLenum type,
			      const GLvoid * pixels,
			      const struct gl_pixelstore_attrib *packing,
			      struct gl_texture_object *texObj,
			      struct gl_texture_image *texImage);
extern GLboolean fxDDTestProxyTexImage (GLcontext *ctx, GLenum target,
                                        GLint level, GLint internalFormat,
                                        GLenum format, GLenum type,
                                        GLint width, GLint height,
                                        GLint depth, GLint border);
extern void fxDDTexEnv(GLcontext *, GLenum, GLenum, const GLfloat *);
extern void fxDDTexParam(GLcontext *, GLenum, struct gl_texture_object *,
			 GLenum, const GLfloat *);
extern void fxDDTexBind(GLcontext *, GLenum, struct gl_texture_object *);
extern struct gl_texture_object *fxDDNewTextureObject( GLcontext *ctx, GLuint name, GLenum target );
extern void fxDDTexDel(GLcontext *, struct gl_texture_object *);
extern GLboolean fxDDIsTextureResident(GLcontext *, struct gl_texture_object *);
extern void fxDDTexPalette(GLcontext *, struct gl_texture_object *);
extern void fxDDTexUseGlbPalette(GLcontext *, GLboolean);

extern void fxDDEnable(GLcontext *, GLenum, GLboolean);
extern void fxDDAlphaFunc(GLcontext *, GLenum, GLfloat);
extern void fxDDBlendFuncSeparate(GLcontext *, GLenum, GLenum, GLenum, GLenum);
extern void fxDDBlendEquationSeparate(GLcontext *, GLenum, GLenum);
extern void fxDDDepthMask(GLcontext *, GLboolean);
extern void fxDDDepthFunc(GLcontext *, GLenum);
extern void fxDDStencilFuncSeparate (GLcontext *ctx, GLenum face, GLenum func, GLint ref, GLuint mask);
extern void fxDDStencilMaskSeparate (GLcontext *ctx, GLenum face, GLuint mask);
extern void fxDDStencilOpSeparate (GLcontext *ctx, GLenum face, GLenum sfail, GLenum zfail, GLenum zpass);

extern void fxDDInitExtensions(GLcontext * ctx);

extern void fxTMInit(fxMesaContext ctx);
extern void fxTMClose(fxMesaContext ctx);
extern void fxTMRestoreTextures_NoLock(fxMesaContext ctx);
extern void fxTMMoveInTM(fxMesaContext, struct gl_texture_object *, GLint);
extern void fxTMMoveOutTM(fxMesaContext, struct gl_texture_object *);
#define fxTMMoveOutTM_NoLock fxTMMoveOutTM
extern void fxTMFreeTexture(fxMesaContext, struct gl_texture_object *);
extern void fxTMReloadMipMapLevel(fxMesaContext, struct gl_texture_object *,
				  GLint);
extern void fxTMReloadSubMipMapLevel(fxMesaContext,
				     struct gl_texture_object *, GLint, GLint,
				     GLint);
extern int fxTMCheckStartAddr (fxMesaContext fxMesa, GLint tmu, tfxTexInfo *ti);

extern void fxTexGetFormat(GLcontext *, GLenum, GrTextureFormat_t *, GLint *); /* [koolsmoky] */

extern int fxTexGetInfo(int, int, GrLOD_t *, GrAspectRatio_t *,
			float *, float *, int *, int *);

extern void fxDDScissor(GLcontext * ctx,
			GLint x, GLint y, GLsizei w, GLsizei h);
extern void fxDDFogfv(GLcontext * ctx, GLenum pname, const GLfloat * params);
extern void fxDDColorMask(GLcontext * ctx,
			  GLboolean r, GLboolean g, GLboolean b, GLboolean a);

extern void fxDDWriteDepthSpan(GLcontext * ctx, GLuint n, GLint x, GLint y,
			       const GLuint depth[], const GLubyte mask[]);

extern void fxDDReadDepthSpan(GLcontext * ctx, GLuint n, GLint x, GLint y,
			      GLuint depth[]);

extern void fxDDWriteDepthPixels(GLcontext * ctx, GLuint n,
				 const GLint x[], const GLint y[],
				 const GLuint depth[], const GLubyte mask[]);

extern void fxDDReadDepthPixels(GLcontext * ctx, GLuint n,
				const GLint x[], const GLint y[],
				GLuint depth[]);

extern void fxDDShadeModel(GLcontext * ctx, GLenum mode);

extern void fxDDCullFace(GLcontext * ctx, GLenum mode);
extern void fxDDFrontFace(GLcontext * ctx, GLenum mode);

extern void fxPrintRenderState(const char *msg, GLuint state);
extern void fxPrintHintState(const char *msg, GLuint state);

extern int fxDDInitFxMesaContext(fxMesaContext fxMesa);
extern void fxDDDestroyFxMesaContext(fxMesaContext fxMesa);


extern void fxSetScissorValues(GLcontext * ctx);
extern void fxTMMoveInTM_NoLock(fxMesaContext fxMesa,
				struct gl_texture_object *tObj, GLint where);

extern void fxCheckIsInHardware(GLcontext *ctx);

/* fxsetup:
 * semi-private functions
 */
void fxSetupCull (GLcontext * ctx);
void fxSetupScissor (GLcontext * ctx);
void fxSetupColorMask (GLcontext * ctx);
void fxSetupBlend (GLcontext *ctx);
void fxSetupDepthTest (GLcontext *ctx);
void fxSetupTexture (GLcontext *ctx);
void fxSetupStencil (GLcontext *ctx);
void fxSetupStencilFace (GLcontext *ctx, GLint face);

/* Flags for software fallback cases */
#define FX_FALLBACK_TEXTURE_MAP		0x0001
#define FX_FALLBACK_DRAW_BUFFER		0x0002
#define FX_FALLBACK_SPECULAR		0x0004
#define FX_FALLBACK_STENCIL		0x0008
#define FX_FALLBACK_RENDER_MODE		0x0010
#define FX_FALLBACK_LOGICOP		0x0020
#define FX_FALLBACK_TEXTURE_ENV		0x0040
#define FX_FALLBACK_TEXTURE_BORDER	0x0080
#define FX_FALLBACK_COLORMASK		0x0100
#define FX_FALLBACK_BLEND		0x0200
#define FX_FALLBACK_TEXTURE_MULTI	0x0400

extern GLuint fx_check_IsInHardware(GLcontext *ctx);

/***
 *** CNORM: clamp float to [0,1] and map to float in [0,255]
 ***/
#if defined(USE_IEEE) && !defined(DEBUG)
#define IEEE_0996 0x3f7f0000	/* 0.996 or so */
#define CNORM(N, F)				\
        do {					\
           fi_type __tmp;			\
           __tmp.f = (F);			\
           if (__tmp.i < 0)			\
              N = 0;				\
           else if (__tmp.i >= IEEE_0996)	\
              N = 255.0f;			\
           else {				\
              N = (F) * 255.0f;			\
           }					\
        } while (0)
#else
#define CNORM(n, f) \
	n = (CLAMP((f), 0.0F, 1.0F) * 255.0F)
#endif

/* run-time debugging */
#ifndef FX_DEBUG
#define FX_DEBUG 0
#endif
#if FX_DEBUG
extern int TDFX_DEBUG;
#else
#define TDFX_DEBUG		0
#endif

/* dirty hacks */
#define FX_RESCALE_BIG_TEXURES_HACK   1
#define FX_COMPRESS_S3TC_AS_FXT1_HACK 1

#endif
