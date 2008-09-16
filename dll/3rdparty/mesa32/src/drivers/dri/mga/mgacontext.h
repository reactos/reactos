/* $XFree86: xc/lib/GL/mesa/src/drv/mga/mgacontext.h,v 1.7 2002/12/16 16:18:52 dawes Exp $*/
/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef MGALIB_INC
#define MGALIB_INC

#include <stdint.h>
#include "drm.h"
#include "mga_drm.h"
#include "dri_util.h"
#include "mtypes.h"
#include "xf86drm.h"
#include "mm.h"
#include "colormac.h"
#include "texmem.h"
#include "macros.h"
#include "xmlconfig.h"

#define MGA_SET_FIELD(reg,mask,val)  reg = ((reg) & (mask)) | ((val) & ~(mask))
#define MGA_FIELD(field,val) (((val) << (field ## _SHIFT)) & ~(field ## _MASK))
#define MGA_GET_FIELD(field, val) ((val & ~(field ## _MASK)) >> (field ## _SHIFT))

#define MGA_IS_G200(mmesa) (mmesa->mgaScreen->chipset == MGA_CARD_TYPE_G200)
#define MGA_IS_G400(mmesa) (mmesa->mgaScreen->chipset == MGA_CARD_TYPE_G400)


/* SoftwareFallback
 *    - texture env GL_BLEND -- can be fixed
 *    - 1D and 3D textures
 *    - incomplete textures
 *    - GL_DEPTH_FUNC == GL_NEVER not in h/w
 */
#define MGA_FALLBACK_TEXTURE        0x1
#define MGA_FALLBACK_DRAW_BUFFER    0x2
#define MGA_FALLBACK_READ_BUFFER    0x4
#define MGA_FALLBACK_BLEND          0x8
#define MGA_FALLBACK_RENDERMODE     0x10
#define MGA_FALLBACK_STENCIL        0x20
#define MGA_FALLBACK_DEPTH          0x40
#define MGA_FALLBACK_BORDER_MODE    0x80
#define MGA_FALLBACK_DISABLE        0x100


/* Use the templated vertex formats:
 */
#define TAG(x) mga##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

typedef struct mga_context_t mgaContext;
typedef struct mga_context_t *mgaContextPtr;

typedef void (*mga_tri_func)( mgaContextPtr, mgaVertex *, mgaVertex *,
			       mgaVertex * );
typedef void (*mga_line_func)( mgaContextPtr, mgaVertex *, mgaVertex * );
typedef void (*mga_point_func)( mgaContextPtr, mgaVertex * );



/* Texture environment color
 */
#define RGB_ZERO(c)   (((c) & 0xffffff) == 0x000000)
#define RGB_ONE(c)    (((c) & 0xffffff) == 0xffffff)
#define ALPHA_ZERO(c) (((c) >> 24) == 0x00)
#define ALPHA_ONE(c)  (((c) >> 24) == 0xff)
#define RGBA_EQUAL(c) ((c) == PACK_COLOR_8888( (c) & 0xff, (c) & 0xff, \
                                               (c) & 0xff, (c) & 0xff ))

struct mga_texture_object_s;
struct mga_screen_private_s;

#define G200_TEX_MAXLEVELS 5
#define G400_TEX_MAXLEVELS 11

typedef struct mga_texture_object_s
{
   driTextureObject   base;

   /* The G200 only has the ability to use 5 mipmap levels (including the
    * base level).  The G400 does not have this restriction, but it still
    * only has 5 offset pointers in the hardware.  The trick on the G400 is
    * upto the first 4 offset pointers point to mipmap levels.  The last
    * offset pointer tells how large the preceeding mipmap is.  This value is
    * then used to determine where the remaining mipmaps are.
    * 
    * For example, if the first offsets[0] through offsets[2] are used as
    * pointers, then offset[3] will be the size of the mipmap pointed to by
    * offsets[2].  So mipmap level 3 will be at (offsets[2]+offsets[3]).  For
    * each successive mipmap level, offsets[3] is divided by 4 and added to
    * the previous address.  So mipmap level 4 will be at 
    * (offsets[2]+offsets[3]+(offsets[3] / 4)).
    * 
    * The last pointer is selected by setting TO_texorgoffsetsel in its
    * pointer.  In the previous example, offset[2] would have
    * TO_texorgoffsetsel or'ed in before writing it to the hardware.
    * 
    * In the current driver all of the mipmaps are packed together linearly
    * with mipmap level 0.  Therefore offsets[0] points to the base of the
    * texture (and has TO_texorgoffsetsel or'ed in), and offsets[1] is the
    * size of the base texture.
    *
    * There is a possible optimization available here.  At times the driver
    * may not be able to allocate a single block of memory for the complete
    * texture without ejecting some other textures from memory.  It may be
    * possible to put some of the lower mipmap levels (i.e., the larger
    * mipmaps) in memory separate from the higher levels.
    *
    * The implementation should be fairly obvious, but getting "right" would
    * likely be non-trivial.  A first allocation for the entire texture would
    * be attempted with a flag that says "don't eject other textures."  If
    * that failed, an additional allocation would be attmpted for just the
    * base map.  The process would repeat with the block of lower maps.  The
    * tricky parts would be in detecting when some of the levels had been
    * ejected from texture memory by other textures and preventing the
    * 4th allocation (for all the smallest mipmap levels) from kicking out
    * any of the first three.
    * 
    * This array holds G400_TEX_MAXLEVELS pointers to remove an if-statement
    * in a loop in mgaSetTexImages.  Values past G200_TEX_MAXLEVELS are not
    * used.
    */
   GLuint             offsets[G400_TEX_MAXLEVELS];

   int                texelBytes;
   GLuint             age;

   drm_mga_texture_regs_t setup;

   /* If one texture dimension wraps with GL_CLAMP and the other with
    * GL_CLAMP_TO_EDGE, we have to fallback to software.  We would also have
    * to fallback for GL_CLAMP_TO_BORDER.
    */
   GLboolean          border_fallback;
   /* Depending on multitxturing and environment color
    * GL_BLEND may have to be a software fallback.
    */
   GLboolean texenv_fallback;
} mgaTextureObject_t;

struct mga_hw_state {
   GLuint   specen;
   GLuint   cull;
   GLuint   cull_dualtex;
   GLuint   stencil;
   GLuint   stencilctl;
   GLuint   stencil_enable;
   GLuint   zmode;
   GLuint   rop;
   GLuint   alpha_func;
   GLuint   alpha_func_enable;
   GLuint   blend_func;
   GLuint   blend_func_enable;
   GLuint   alpha_sel;
};

struct mga_context_t {

   GLcontext *glCtx;
   unsigned int lastStamp;		/* fullscreen breaks dpriv->laststamp,
					 * need to shadow it here. */

   /* Hardware state management
    */
   struct mga_hw_state hw;

   /* Bookkeeping for texturing
    */
   unsigned           nr_heaps;
   driTexHeap       * texture_heaps[ MGA_NR_TEX_HEAPS ];
   driTextureObject   swapped;

   struct mga_texture_object_s *CurrentTexObj[2];


   /* Map GL texture units onto hardware.
    */
   GLuint tmu_source[2];
   
   int texture_depth;

   /* Manage fallbacks
    */
   GLuint Fallback;  

   /* Texture environment color.
    */
   unsigned int envcolor[2];
   GLboolean fcol_used;
   GLboolean force_dualtex;

   /* Rasterization state 
    */
   GLuint SetupNewInputs;
   GLuint SetupIndex;
   GLuint RenderIndex;
   
   GLuint hw_primitive;
   GLenum raster_primitive;
   GLenum render_primitive;

   GLubyte *verts;
   GLint vertex_stride_shift;
   GLuint vertex_format;		
   GLuint vertex_size;

   /* Fallback rasterization functions 
    */
   mga_point_func draw_point;
   mga_line_func draw_line;
   mga_tri_func draw_tri;


   /* Manage driver and hardware state
    */
   GLuint        NewGLState; 
   GLuint        dirty;

   drm_mga_context_regs_t setup;

   GLuint        ClearColor;
   GLuint        ClearDepth;
   GLuint        poly_stipple;
   GLfloat       depth_scale;

   GLuint        depth_clear_mask;
   GLuint        stencil_clear_mask;
   GLuint        hw_stencil;
   GLuint        haveHwStipple;
   GLfloat       hw_viewport[16];

   /* Dma buffers
    */
   drmBufPtr  vertex_dma_buffer;
   drmBufPtr  iload_buffer;

   /* VBI
    */
   GLuint vbl_seq;
   GLuint vblank_flags;

   int64_t swap_ust;
   int64_t swap_missed_ust;

   GLuint swap_count;
   GLuint swap_missed_count;

   uint32_t last_frame_fence;

   /* Drawable, cliprect and scissor information
    */
   int dirty_cliprects;		/* which sets of cliprects are uptodate? */
   int draw_buffer;		/* which buffer are we rendering to */
   unsigned int drawOffset;		/* draw buffer address in  space */
   int readOffset;
   int drawX, drawY;		/* origin of drawable in draw buffer */
   int lastX, lastY;		/* detect DSTORG bug */
   GLuint numClipRects;		/* cliprects for the draw buffer */
   drm_clip_rect_t *pClipRects;
   drm_clip_rect_t draw_rect;
   drm_clip_rect_t scissor_rect;
   int scissor;

   drm_clip_rect_t tmp_boxes[2][MGA_NR_SAREA_CLIPRECTS];


   /* Texture aging and DMA based aging.
    */
   unsigned int texAge[MGA_NR_TEX_HEAPS];/* texture LRU age  */
   unsigned int dirtyAge;		/* buffer age for synchronization */

   GLuint primary_offset;

   /* Mirrors of some DRI state.
    */
   drm_context_t hHWContext;
   drm_hw_lock_t *driHwLock;
   int driFd;
   __DRIdrawablePrivate *driDrawable;
   __DRIdrawablePrivate *driReadable;

   __DRIscreenPrivate *driScreen;
   struct mga_screen_private_s *mgaScreen;
   drm_mga_sarea_t *sarea;

   /* Configuration cache
    */
   driOptionCache optionCache;
};

#define MGA_CONTEXT(ctx) ((mgaContextPtr)(ctx->DriverCtx))




/* ================================================================
 * Debugging:
 */
#define DO_DEBUG		1

#if DO_DEBUG
extern int MGA_DEBUG;
#else
#define MGA_DEBUG		0
#endif

#define DEBUG_VERBOSE_MSG	0x01
#define DEBUG_VERBOSE_DRI	0x02
#define DEBUG_VERBOSE_IOCTL	0x04
#define DEBUG_VERBOSE_TEXTURE   0x08
#define DEBUG_VERBOSE_FALLBACK	0x10

static __inline__ GLuint mgaPackColor(GLuint cpp,
				      GLubyte r, GLubyte g,
				      GLubyte b, GLubyte a)
{
   switch (cpp) {
   case 2:
      return PACK_COLOR_565( r, g, b );
   case 4:
      return PACK_COLOR_8888( a, r, g, b );
   default:
      return 0;
   }
}


/*
 * Subpixel offsets for window coordinates:
 */
#define SUBPIXEL_X (-0.5F)
#define SUBPIXEL_Y (-0.5F + 0.125)


#define MGA_WA_TRIANGLES     0x18000000
#define MGA_WA_TRISTRIP_T0   0x02010200
#define MGA_WA_TRIFAN_T0     0x01000408
#define MGA_WA_TRISTRIP_T0T1 0x02010400
#define MGA_WA_TRIFAN_T0T1   0x01000810

#endif
