/*
 * Copyright 2002 by Alan Hourihane, Sychdyn, North Wales, UK.
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
 * Authors:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 *
 * Trident CyberBladeXP driver.
 *
 */
#ifndef _TRIDENT_CONTEXT_H_
#define _TRIDENT_CONTEXT_H_

#include "dri_util.h"
#include "macros.h"
#include "mtypes.h"
#include "drm.h"
#include "mm.h"

#define SUBPIXEL_X (0.0F)
#define SUBPIXEL_Y (0.125F)

#define _TRIDENT_NEW_VERTEX (_NEW_TEXTURE |		\
			   _DD_NEW_TRI_UNFILLED |	\
			   _DD_NEW_TRI_LIGHT_TWOSIDE)

#define TRIDENT_FALLBACK_TEXTURE	0x01
#define TRIDENT_FALLBACK_DRAW_BUFFER	0x02

#define TRIDENT_NEW_CLIP		0x01

#define TRIDENT_UPLOAD_COMMAND_D	0x00000001
#define TRIDENT_UPLOAD_CONTEXT		0x04000000
#define TRIDENT_UPLOAD_CLIPRECTS	0x80000000

#define TAG(x) trident##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

/* these require that base be dword-aligned */
static inline void MMIO_OUT32(unsigned char *base, unsigned int offset,
                              unsigned int val)
{
    unsigned int *addr = (unsigned int *)(base + offset);
    *addr = val;
}

static inline unsigned int MMIO_IN32(unsigned char *base, unsigned int offset)
{
    unsigned int *addr = (unsigned int *)(base + offset);
    return *addr;
}

#define MMIO_OUT8(base, offset, val) *((base) + (offset)) = (val)
#define MMIO_IN8(base, offset) *((base) + (offset))

struct trident_context;
typedef struct trident_context tridentContextRec;
typedef struct trident_context *tridentContextPtr;

typedef void (*trident_quad_func)( tridentContextPtr, 
				 const tridentVertex *, 
				 const tridentVertex *,
				 const tridentVertex *,
				 const tridentVertex * );
typedef void (*trident_tri_func)( tridentContextPtr, 
				const tridentVertex *, 
				const tridentVertex *,
				const tridentVertex * );
typedef void (*trident_line_func)( tridentContextPtr, 
				 const tridentVertex *, 
				 const tridentVertex * );
typedef void (*trident_point_func)( tridentContextPtr, 
				  const tridentVertex * );

typedef struct {
   drm_handle_t handle;			/* Handle to the DRM region */
   drmSize size;			/* Size of the DRM region */
   unsigned char *map;			/* Mapping of the DRM region */
} tridentRegionRec, *tridentRegionPtr;

typedef struct {
    __DRIscreenPrivate *driScreen; /* Back pointer to DRI screen */

    drmBufMapPtr buffers;

    unsigned int frontOffset;
    unsigned int frontPitch;
    unsigned int backOffset;
    unsigned int backPitch;
    unsigned int depthOffset;
    unsigned int depthPitch;
    unsigned int width;
    unsigned int height;
    unsigned int cpp;

#if 0
    unsigned int sarea_priv_offset;
#endif

    tridentRegionRec mmio;
} tridentScreenRec, *tridentScreenPtr;

/**
 * tridentRenderbuffer, derived from Mesa's gl_renderbuffer
 */
typedef struct {
   struct gl_renderbuffer Base;
   /* XXX per-window info should go here */
   int foo, bar;
} tridentRenderbuffer;


struct trident_context {
	GLcontext 		*glCtx;		/* Mesa context */

	__DRIcontextPrivate	*driContext;
	__DRIscreenPrivate	*driScreen;
	__DRIdrawablePrivate	*driDrawable;

	GLuint 			new_gl_state;
	GLuint 			new_state;
	GLuint 			dirty;

#if 0
  	drm_trident_sarea_t	*sarea; 
#endif

        /* Temporaries for translating away float colors:
	 */
        struct gl_client_array UbyteColor;
        struct gl_client_array UbyteSecondaryColor;

   	/* Mirrors of some DRI state
    	 */
	int lastStamp;		        /* mirror driDrawable->lastStamp */

   	drm_context_t hHWContext;
   	drmLock *driHwLock;
   	int driFd;

   	tridentScreenPtr tridentScreen;	/* Screen private DRI data */

  	/* Visual, drawable, cliprect and scissor information
    	 */
   	GLenum DrawBuffer;
   	GLint drawOffset, drawPitch;
   	GLint drawX, drawY;             /* origin of drawable in draw buffer */
   	GLint readOffset, readPitch;

   	GLuint numClipRects;		/* Cliprects for the draw buffer */
   	drm_clip_rect_t *pClipRects;

   	GLint scissor;
   	drm_clip_rect_t ScissorRect;	/* Current software scissor */

   	GLuint Fallback;
	GLuint RenderIndex;
	GLuint SetupNewInputs;
	GLuint SetupIndex;
	GLfloat hw_viewport[16];
	GLfloat	depth_scale;
	GLuint vertex_format;
	GLuint vertex_size;
	GLuint vertex_stride_shift;
	GLubyte *verts;

	GLint tmu_source[2];

	GLuint hw_primitive;
	GLenum render_primitive;

   	trident_point_func    draw_point;
   	trident_line_func     draw_line;
   	trident_tri_func      draw_tri;
   	trident_quad_func     draw_quad;

#if 0
   	gammaTextureObjectPtr CurrentTexObj[2];
   	struct gamma_texture_object_t TexObjList;
   	struct gamma_texture_object_t SwappedOut; 
	GLenum TexEnvImageFmt[2];

	struct mem_block *texHeap;

   	int lastSwap;
   	int texAge;
   	int ctxAge;
   	int dirtyAge;
        int lastStamp;
#endif

	/* Chip state */
	
	int	commandD;

	/* Context State */
	
	int	ClearColor;
};

void tridentDDInitExtensions( GLcontext *ctx );
void tridentDDInitDriverFuncs( GLcontext *ctx );
void tridentDDInitSpanFuncs( GLcontext *ctx );
void tridentDDInitState( tridentContextPtr tmesa );
void tridentInitHW( tridentContextPtr tmesa );
void tridentDDInitStateFuncs( GLcontext *ctx );
void tridentDDInitTextureFuncs( GLcontext *ctx );
void tridentDDInitTriFuncs( GLcontext *ctx );

extern void tridentBuildVertices( GLcontext *ctx, 
				GLuint start, 
				GLuint count,
				GLuint newinputs );
extern void tridentInitVB( GLcontext *ctx );
extern void tridentCopyBuffer( const __DRIdrawablePrivate *dPriv );
extern void tridentFallback( tridentContextPtr tmesa, GLuint bit,
                             GLboolean mode );
extern void tridentCheckTexSizes( GLcontext *ctx );
extern void tridentChooseVertexState( GLcontext *ctx );
extern void tridentDDUpdateHWState( GLcontext *ctx );
extern void tridentUploadHwStateLocked( tridentContextPtr tmesa );

#define TRIDENT_CONTEXT(ctx)		((tridentContextPtr)(ctx->DriverCtx))

#endif /* _TRIDENT_CONTEXT_H_ */
