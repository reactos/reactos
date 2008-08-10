/* $XFree86: xc/lib/GL/mesa/src/drv/gamma/gamma_context.h,v 1.6 2002/12/16 16:18:50 dawes Exp $ */
/*
 * Copyright 2001 by Alan Hourihane.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@tungstengraphics.com>
 *
 */

#ifndef _GAMMA_CONTEXT_H_
#define _GAMMA_CONTEXT_H_

#include "dri_util.h"
#include "drm.h"
#include "drm_sarea.h"
#include "colormac.h"
#include "gamma_regs.h"
#include "gamma_macros.h"
#include "gamma_screen.h"
#include "macros.h"
#include "mtypes.h"
#include "glint_dri.h"
#include "mm.h"

typedef union {
    unsigned int i;
    float        f;
} dmaBufRec, *dmaBuf;

/* Flags for context */
#define GAMMA_FRONT_BUFFER    0x00000001
#define GAMMA_BACK_BUFFER     0x00000002
#define GAMMA_DEPTH_BUFFER    0x00000004
#define GAMMA_STENCIL_BUFFER  0x00000008
#define GAMMA_ACCUM_BUFFER    0x00000010

#define GAMMA_MAX_TEXTURE_SIZE    2048

/* These are the minimum requirements and should probably be increased */
#define MAX_MODELVIEW_STACK    16
#define MAX_PROJECTION_STACK    2
#define MAX_TEXTURE_STACK       2

extern void	  gammaDDUpdateHWState(GLcontext *ctx);
extern gammaScreenPtr	  gammaCreateScreen(__DRIscreenPrivate *sPriv);
extern void	  gammaDestroyScreen(__DRIscreenPrivate *sPriv);
extern GLboolean gammaCreateContext( const __GLcontextModes *glVisual,
                                     __DRIcontextPrivate *driContextPriv,
                                     void *sharedContextPrivate);

#define GAMMA_UPLOAD_ALL	0xffffffff
#define GAMMA_UPLOAD_CLIPRECTS	0x00000002
#define GAMMA_UPLOAD_ALPHA	0x00000004
#define GAMMA_UPLOAD_BLEND	0x00000008
#define GAMMA_UPLOAD_DEPTH	0x00000010
#define GAMMA_UPLOAD_VIEWPORT	0x00000020
#define GAMMA_UPLOAD_SHADE	0x00000040
#define GAMMA_UPLOAD_CLIP	0x00000080
#define GAMMA_UPLOAD_MASKS	0x00000100
#define GAMMA_UPLOAD_WINDOW	0x00000200 /* defunct */
#define GAMMA_UPLOAD_GEOMETRY	0x00000400
#define GAMMA_UPLOAD_POLYGON	0x00000800
#define GAMMA_UPLOAD_DITHER	0x00001000
#define GAMMA_UPLOAD_LOGICOP	0x00002000
#define GAMMA_UPLOAD_FOG	0x00004000
#define GAMMA_UPLOAD_LIGHT	0x00008000
#define GAMMA_UPLOAD_CONTEXT	0x00010000
#define GAMMA_UPLOAD_TEX0	0x00020000
#define GAMMA_UPLOAD_STIPPLE	0x00040000
#define GAMMA_UPLOAD_TRANSFORM	0x00080000
#define GAMMA_UPLOAD_LINEMODE	0x00100000
#define GAMMA_UPLOAD_POINTMODE	0x00200000
#define GAMMA_UPLOAD_TRIMODE	0x00400000

#define GAMMA_NEW_CLIP		0x00000001
#define GAMMA_NEW_WINDOW	0x00000002
#define GAMMA_NEW_CONTEXT	0x00000004
#define GAMMA_NEW_TEXTURE	0x00000008 /* defunct */
#define GAMMA_NEW_ALPHA		0x00000010
#define GAMMA_NEW_DEPTH		0x00000020
#define GAMMA_NEW_MASKS		0x00000040
#define GAMMA_NEW_POLYGON	0x00000080
#define GAMMA_NEW_CULL		0x00000100
#define GAMMA_NEW_LOGICOP	0x00000200
#define GAMMA_NEW_FOG		0x00000400
#define GAMMA_NEW_LIGHT		0x00000800
#define GAMMA_NEW_STIPPLE	0x00001000
#define GAMMA_NEW_ALL		0xffffffff

#define GAMMA_FALLBACK_TRI	0x00000001
#define GAMMA_FALLBACK_TEXTURE	0x00000002

#define FLUSH_BATCH(gmesa) do {						\
	/*FLUSH_DMA_BUFFER(gmesa);*/					\
} while(0)

struct gamma_context;
typedef struct gamma_context gammaContextRec;
typedef struct gamma_context *gammaContextPtr;
typedef struct gamma_texture_object_t *gammaTextureObjectPtr;

#define VALID_GAMMA_TEXTURE_OBJECT(tobj)  (tobj) 

#define GAMMA_TEX_MAXLEVELS 12  /* 2K x 2K */

/* For shared texture space managment, these texture objects may also
 * be used as proxies for regions of texture memory containing other
 * client's textures.  Such proxy textures (not to be confused with GL
 * proxy textures) are subject to the same LRU aging we use for our
 * own private textures, and thus we have a mechanism where we can
 * fairly decide between kicking out our own textures and those of
 * other clients.
 *
 * Non-local texture objects have a valid MemBlock to describe the
 * region managed by the other client, and can be identified by
 * 't->globj == 0' 
 */
struct gamma_texture_object_t {
   struct gamma_texture_object_t *next, *prev;

   GLuint age;   
   struct gl_texture_object *globj;
     
   int Pitch;
   int Height;
   int texelBytes;
   int totalSize;
   int bound;

   struct mem_block *MemBlock;   
   char * BufAddr;
   
   GLuint min_level;
   GLuint max_level;
   GLuint dirty_images;

   GLint firstLevel, lastLevel;  /* upload tObj->Image[0][first .. lastLevel] */

   struct { 
      const struct gl_texture_image *image;
      int offset;		/* into BufAddr */
      int height;
      int internalFormat;
   } image[GAMMA_TEX_MAXLEVELS];

   u_int32_t TextureBaseAddr[GAMMA_TEX_MAXLEVELS];
   u_int32_t TextureAddressMode;
   u_int32_t TextureColorMode;
   u_int32_t TextureFilterMode;
   u_int32_t TextureFormat;
   u_int32_t TextureReadMode;
   u_int32_t TextureBorderColor;
};		

#define GAMMA_NO_PALETTE        0x0
#define GAMMA_USE_PALETTE       0x1
#define GAMMA_UPDATE_PALETTE    0x2
#define GAMMA_FALLBACK_PALETTE  0x4

void gammaUpdateTextureState( GLcontext *ctx );

void gammaDestroyTexObj( gammaContextPtr gmesa, gammaTextureObjectPtr t );
void gammaSwapOutTexObj( gammaContextPtr gmesa, gammaTextureObjectPtr t );
void gammaUploadTexImages( gammaContextPtr gmesa, gammaTextureObjectPtr t );

void gammaResetGlobalLRU( gammaContextPtr gmesa );
void gammaUpdateTexLRU( gammaContextPtr gmesa, gammaTextureObjectPtr t );
void gammaTexturesGone( gammaContextPtr gmesa, 
		       GLuint start, GLuint end, 
		       GLuint in_use ); 

void gammaEmitHwState( gammaContextPtr gmesa );
void gammaDDInitExtensions( GLcontext *ctx );
void gammaDDInitDriverFuncs( GLcontext *ctx );
void gammaDDInitSpanFuncs( GLcontext *ctx );
void gammaDDInitState( gammaContextPtr gmesa );
void gammaInitHW( gammaContextPtr gmesa );
void gammaDDInitStateFuncs( GLcontext *ctx );
void gammaDDInitTextureFuncs( struct dd_function_table *table );
void gammaInitTextureObjects( GLcontext *ctx );
void gammaDDInitTriFuncs( GLcontext *ctx );

void gammaUpdateWindow( GLcontext *ctx );
void gammaUpdateViewportOffset( GLcontext *ctx );

void gammaPrintLocalLRU( gammaContextPtr gmesa );
void gammaPrintGlobalLRU( gammaContextPtr gmesa );

extern void gammaFallback( gammaContextPtr gmesa, GLuint bit, GLboolean mode );
#define FALLBACK( imesa, bit, mode ) gammaFallback( imesa, bit, mode )

/* Use the templated vertex formats.  Only one of these is used in gamma.
 */
#define TAG(x) gamma##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

typedef void (*gamma_quad_func)( gammaContextPtr, 
				 const gammaVertex *, 
				 const gammaVertex *,
				 const gammaVertex *,
				 const gammaVertex * );
typedef void (*gamma_tri_func)( gammaContextPtr, 
				const gammaVertex *, 
				const gammaVertex *,
				const gammaVertex * );
typedef void (*gamma_line_func)( gammaContextPtr, 
				 const gammaVertex *, 
				 const gammaVertex * );
typedef void (*gamma_point_func)( gammaContextPtr, 
				  const gammaVertex * );


struct gamma_context {
	GLcontext 		*glCtx;		/* Mesa context */

	__DRIcontextPrivate	*driContext;
	__DRIscreenPrivate	*driScreen;
	__DRIdrawablePrivate	*driDrawable;

	GLuint 			new_gl_state;
	GLuint 			new_state;
	GLuint 			dirty;

  	GLINTSAREADRIPtr        sarea; 

   	/* Mirrors of some DRI state
    	 */
   	drm_context_t hHWContext;
   	drm_hw_lock_t *driHwLock;
   	int driFd;

   	GLuint numClipRects;		   /* Cliprects for the draw buffer */
   	drm_clip_rect_t *pClipRects;

    	dmaBuf              buf;           /* DMA buffer for regular cmds */
    	int                 bufIndex;
    	int                 bufSize;
    	int                 bufCount;

    	dmaBuf              WCbuf;         /* DMA buffer for window changed cmds */
    	int                 WCbufIndex;
    	int                 WCbufSize;
    	int                 WCbufCount;

   	gammaScreenPtr gammaScreen;		/* Screen private DRI data */

	int			drawOffset;
	int			readOffset;

   	gamma_point_func    draw_point;
   	gamma_line_func     draw_line;
   	gamma_tri_func      draw_tri;
   	gamma_quad_func     draw_quad;

   	GLuint Fallback;
	GLuint RenderIndex;
	GLuint SetupNewInputs;
	GLuint SetupIndex;

	GLuint vertex_format;
	GLuint vertex_size;
	GLuint vertex_stride_shift;
	GLubyte *verts;

	GLfloat hw_viewport[16];
	GLuint hw_primitive;
	GLenum render_primitive;

	GLfloat	depth_scale;

   	gammaTextureObjectPtr CurrentTexObj[2];
   	struct gamma_texture_object_t TexObjList;
   	struct gamma_texture_object_t SwappedOut; 
	GLenum TexEnvImageFmt[2];

	struct mem_block *texHeap;

   	unsigned int lastSwap;
   	int texAge;
   	int ctxAge;
   	int dirtyAge;
   	unsigned int lastStamp;
   

    	u_int32_t 		ClearColor;
	u_int32_t		Color;
	u_int32_t		DitherMode;
    	u_int32_t		ClearDepth;
	u_int32_t		FogMode;
	u_int32_t		AreaStippleMode;
	u_int32_t		LBReadFormat;
	u_int32_t		LBWriteFormat;
	u_int32_t		LineMode;
	u_int32_t		PointMode;
	u_int32_t		TriangleMode;
	u_int32_t		AntialiasMode;
	GLfloat			ViewportScaleX;
	GLfloat			ViewportScaleY;
	GLfloat			ViewportScaleZ;
	GLfloat			ViewportOffsetX;
	GLfloat			ViewportOffsetY;
	GLfloat			ViewportOffsetZ;
    int                 MatrixMode;
    int                 DepthMode;
    int			TransformMode;
    int                 LBReadMode;
    int                 FBReadMode;
    int                 FBWindowBase;
    int                 LBWindowBase;
    int                 ColorDDAMode;
    int                 GeometryMode;
    int                 AlphaTestMode;
    int                 AlphaBlendMode;
    int                 AB_FBReadMode;
    int                 AB_FBReadMode_Save;
    int                 DeltaMode;
    int			ColorMaterialMode;
    int			FBHardwareWriteMask;
    int			MaterialMode;
    int			NormalizeMode;
    int			LightingMode;
    int			Light0Mode;
    int			Light1Mode;
    int			Light2Mode;
    int			Light3Mode;
    int			Light4Mode;
    int			Light5Mode;
    int			Light6Mode;
    int			Light7Mode;
    int			Light8Mode;
    int			Light9Mode;
    int			Light10Mode;
    int			Light11Mode;
    int			Light12Mode;
    int			Light13Mode;
    int			Light14Mode;
    int			Light15Mode;
    int			LogicalOpMode;
    int			ScissorMode;
    int			ScissorMaxXY;
    int			ScissorMinXY;
    int                 Window; /* GID part probably should be in draw priv */
    int                 WindowOrigin;
    int                 x, y, w, h; /* Probably should be in drawable priv */
    int                 FrameCount; /* Probably should be in drawable priv */
    int                 NotClipped; /* Probably should be in drawable priv */
    int                 WindowChanged; /* Probably should be in drawabl... */
    int                 Flags;
    int                 EnabledFlags;
    int                 DepthSize;
    int                 Begin;
    GLenum              ErrorValue;
    int                 Texture1DEnabled;
    int                 Texture2DEnabled;

    float               ModelView[16];
    float               Proj[16];
    float               ModelViewProj[16];
    float               Texture[16];

    float               ModelViewStack[(MAX_MODELVIEW_STACK-1)*16];
    int                 ModelViewCount;
    float               ProjStack[(MAX_PROJECTION_STACK-1)*16];
    int                 ProjCount;
    float               TextureStack[(MAX_TEXTURE_STACK-1)*16];
    int                 TextureCount;
};

static __inline GLuint gammaPackColor( GLuint cpp,
					GLubyte r, GLubyte g,
					GLubyte b, GLubyte a )
{
   switch ( cpp ) {
   case 2:
      return PACK_COLOR_565( r, g, b );
   case 4:
      return PACK_COLOR_8888( a, r, g, b );
   default:
      return 0;
   }
}

#define GAMMA_CONTEXT(ctx)		((gammaContextPtr)(ctx->DriverCtx))

#endif /* _GAMMA_CONTEXT_H_ */
