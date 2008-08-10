/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef INTELCONTEXT_INC
#define INTELCONTEXT_INC



#include "mtypes.h"
#include "drm.h"
#include "texmem.h"

#include "intel_screen.h"
#include "i830_common.h"
#include "tnl/t_vertex.h"

#define TAG(x) intel##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

#define DV_PF_555  (1<<8)
#define DV_PF_565  (2<<8)
#define DV_PF_8888 (3<<8)

struct intel_region;
struct intel_context;

typedef void (*intel_tri_func)(struct intel_context *, intelVertex *, intelVertex *,
							  intelVertex *);
typedef void (*intel_line_func)(struct intel_context *, intelVertex *, intelVertex *);
typedef void (*intel_point_func)(struct intel_context *, intelVertex *);

#define INTEL_FALLBACK_DRAW_BUFFER	 0x1
#define INTEL_FALLBACK_READ_BUFFER	 0x2
#define INTEL_FALLBACK_USER		 0x4
#define INTEL_FALLBACK_RENDERMODE	 0x8
#define INTEL_FALLBACK_TEXTURE   	 0x10

extern void intelFallback( struct intel_context *intel, GLuint bit, GLboolean mode );
#define FALLBACK( intel, bit, mode ) intelFallback( intel, bit, mode )



struct intel_texture_object
{
   struct gl_texture_object base; /* The "parent" object */

   /* The mipmap tree must include at least these levels once
    * validated:
    */
   GLuint firstLevel;
   GLuint lastLevel;

   GLuint dirty_images[6];
   GLuint dirty;

   /* On validation any active images held in main memory or in other
    * regions will be copied to this region and the old storage freed.
    */
   struct intel_mipmap_tree *mt;
};



struct intel_context
{
   GLcontext ctx;		/* the parent class */

   struct {
      void (*destroy)( struct intel_context *intel ); 
      void (*emit_state)( struct intel_context *intel );
      void (*emit_invarient_state)( struct intel_context *intel );
      void (*lost_hardware)( struct intel_context *intel );
      void (*note_fence)( struct intel_context *intel, GLuint fence );
      void (*note_unlock)( struct intel_context *intel );
      void (*update_texture_state)( struct intel_context *intel );

      void (*render_start)( struct intel_context *intel );
      void (*set_draw_region)( struct intel_context *intel, 
			       struct intel_region *draw_region,
			       struct intel_region *depth_region );

      GLuint (*flush_cmd)( void );

      void (*emit_flush)( struct intel_context *intel,
			  GLuint unused );

      void (*aub_commands)( struct intel_context *intel, 
			    GLuint offset,
			    const void *buf,
			    GLuint sz );
      void (*aub_dump_bmp)( struct intel_context *intel, GLuint buffer );
      void (*aub_wrap)( struct intel_context *intel );
      void (*aub_gtt_data)( struct intel_context *intel, 
			    GLuint offset,
			    const void *src,
			    GLuint size,
			    GLuint aubtype, 
			    GLuint aubsubtype);


      void (*reduced_primitive_state)( struct intel_context *intel, GLenum rprim );

      GLboolean (*check_vertex_size)( struct intel_context *intel, GLuint expected );

      void (*invalidate_state)( struct intel_context *intel, GLuint new_state );

      /* Metaops: 
       */
      void (*install_meta_state)( struct intel_context *intel );
      void (*leave_meta_state)( struct intel_context *intel );

      void (*meta_draw_region)( struct intel_context *intel,
				struct intel_region *draw_region,
				struct intel_region *depth_region );

      void (*meta_color_mask)( struct intel_context *intel,
			       GLboolean );
      
      void (*meta_stencil_replace)( struct intel_context *intel,
				    GLuint mask,
				    GLuint clear );

      void (*meta_depth_replace)( struct intel_context *intel );

      void (*meta_texture_blend_replace) (struct intel_context * intel);
      
      void (*meta_no_stencil_write)( struct intel_context *intel );
      void (*meta_no_depth_write)( struct intel_context *intel );
      void (*meta_no_texture)( struct intel_context *intel );
      void (*meta_import_pixel_state) (struct intel_context * intel);
      void (*meta_frame_buffer_texture)( struct intel_context *intel,
					 GLint xoff, GLint yoff );

      void (*meta_draw_quad)(struct intel_context *intel, 
			     GLfloat x0, GLfloat x1,
			     GLfloat y0, GLfloat y1, 
			     GLfloat z,
			     GLubyte red, GLubyte green,
			     GLubyte blue, GLubyte alpha,
			     GLfloat s0, GLfloat s1,
			     GLfloat t0, GLfloat t1);



   } vtbl;

   GLint refcount;   
   GLuint Fallback;
   GLuint NewGLState;
   
   GLuint last_swap_fence;
   GLuint second_last_swap_fence;
   
   GLboolean aub_wrap;
   GLuint stats_wm;

   struct intel_batchbuffer *batch;

   GLubyte clear_chan[4];
   GLuint ClearColor;
   GLuint ClearDepth;

   GLfloat depth_scale;
   GLfloat polygon_offset_scale; /* dependent on depth_scale, bpp */
   GLuint depth_clear_mask;
   GLuint stencil_clear_mask;

   GLboolean hw_stencil;
   GLboolean hw_stipple;
   GLboolean depth_buffer_is_float;
   GLboolean no_hw;
   GLboolean no_rast;
   GLboolean thrashing;
   GLboolean locked;
   GLboolean strict_conformance;
   GLboolean need_flush;


   
   /* AGP memory buffer manager:
    */
   struct bufmgr *bm;


   /* State for intelvb.c and inteltris.c.
    */
   GLenum render_primitive;
   GLenum reduced_primitive;

   struct intel_region *front_region;
   struct intel_region *back_region;
   struct intel_region *draw_region;
   struct intel_region *depth_region;

   /* These refer to the current draw (front vs. back) buffer:
    */
   int drawX;			/* origin of drawable in draw buffer */
   int drawY;
   GLuint numClipRects;		/* cliprects for that buffer */
   drm_clip_rect_t *pClipRects;
   struct gl_texture_object *frame_buffer_texobj;

   GLboolean scissor;
   drm_clip_rect_t draw_rect;
   drm_clip_rect_t scissor_rect;

   drm_context_t hHWContext;
   drmLock *driHwLock;
   int driFd;

   __DRIdrawablePrivate *driDrawable;
   __DRIdrawablePrivate *driReadDrawable;
   __DRIscreenPrivate *driScreen;
   intelScreenPrivate *intelScreen; 
   volatile drmI830Sarea *sarea; 
   
   FILE *aub_file;

   GLuint lastStamp;

   /**
    * Configuration cache
    */
   driOptionCache optionCache;

   /* VBI
    */
   GLuint vbl_seq;
   GLuint vblank_flags;

   int64_t swap_ust;
   int64_t swap_missed_ust;

   GLuint swap_count;
   GLuint swap_missed_count;
};

/* These are functions now:
 */
void LOCK_HARDWARE( struct intel_context *intel );
void UNLOCK_HARDWARE( struct intel_context *intel );


#define SUBPIXEL_X 0.125
#define SUBPIXEL_Y 0.125

/* ================================================================
 * Color packing:
 */

#define INTEL_PACKCOLOR4444(r,g,b,a) \
  ((((a) & 0xf0) << 8) | (((r) & 0xf0) << 4) | ((g) & 0xf0) | ((b) >> 4))

#define INTEL_PACKCOLOR1555(r,g,b,a) \
  ((((r) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((b) & 0xf8) >> 3) | \
    ((a) ? 0x8000 : 0))

#define INTEL_PACKCOLOR565(r,g,b) \
  ((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | (((b) & 0xf8) >> 3))

#define INTEL_PACKCOLOR8888(r,g,b,a) \
  ((a<<24) | (r<<16) | (g<<8) | b)


#define INTEL_PACKCOLOR(format, r,  g,  b, a)		\
(format == DV_PF_555 ? INTEL_PACKCOLOR1555(r,g,b,a) :	\
 (format == DV_PF_565 ? INTEL_PACKCOLOR565(r,g,b) :	\
  (format == DV_PF_8888 ? INTEL_PACKCOLOR8888(r,g,b,a) :	\
   0)))



/* ================================================================
 * From linux kernel i386 header files, copes with odd sizes better
 * than COPY_DWORDS would:
 */
#if defined(i386) || defined(__i386__)
static inline void * __memcpy(void * to, const void * from, size_t n)
{
   int d0, d1, d2;
   __asm__ __volatile__(
      "rep ; movsl\n\t"
      "testb $2,%b4\n\t"
      "je 1f\n\t"
      "movsw\n"
      "1:\ttestb $1,%b4\n\t"
      "je 2f\n\t"
      "movsb\n"
      "2:"
      : "=&c" (d0), "=&D" (d1), "=&S" (d2)
      :"0" (n/4), "q" (n),"1" ((long) to),"2" ((long) from)
      : "memory");
   return (to);
}
#else
#define __memcpy(a,b,c) memcpy(a,b,c)
#endif


/* The system memcpy (at least on ubuntu 5.10) has problems copying
 * to agp (writecombined) memory from a source which isn't 64-byte
 * aligned - there is a 4x performance falloff.
 *
 * The x86 __memcpy is immune to this but is slightly slower
 * (10%-ish) than the system memcpy.
 *
 * The sse_memcpy seems to have a slight cliff at 64/32 bytes, but
 * isn't much faster than x86_memcpy for agp copies.
 * 
 * TODO: switch dynamically.
 */
static inline void *do_memcpy( void *dest, const void *src, size_t n )
{
   if ( (((unsigned long)src) & 63) ||
	(((unsigned long)dest) & 63)) {
      return  __memcpy(dest, src, n);	
   }
   else
      return memcpy(dest, src, n);
}





/* ================================================================
 * Debugging:
 */
extern int INTEL_DEBUG;

#define DEBUG_TEXTURE	0x1
#define DEBUG_STATE	0x2
#define DEBUG_IOCTL	0x4
#define DEBUG_PRIMS	0x8
#define DEBUG_VERTS	0x10
#define DEBUG_FALLBACKS	0x20
#define DEBUG_VERBOSE	0x40
#define DEBUG_DRI       0x80
#define DEBUG_DMA       0x100
#define DEBUG_SANITY    0x200
#define DEBUG_SYNC      0x400
#define DEBUG_SLEEP     0x800
#define DEBUG_PIXEL     0x1000
#define DEBUG_STATS     0x2000
#define DEBUG_TILE      0x4000
#define DEBUG_SINGLE_THREAD   0x8000
#define DEBUG_WM        0x10000
#define DEBUG_URB       0x20000
#define DEBUG_VS        0x40000


#define PCI_CHIP_845_G			0x2562
#define PCI_CHIP_I830_M			0x3577
#define PCI_CHIP_I855_GM		0x3582
#define PCI_CHIP_I865_G			0x2572
#define PCI_CHIP_I915_G			0x2582
#define PCI_CHIP_I915_GM		0x2592
#define PCI_CHIP_I945_G			0x2772
#define PCI_CHIP_I965_G			0x29A2
#define PCI_CHIP_I965_Q			0x2992
#define PCI_CHIP_I965_G_1		0x2982
#define PCI_CHIP_I946_GZ		0x2972
#define PCI_CHIP_I965_GM                0x2A02


/* ================================================================
 * intel_context.c:
 */

extern GLboolean intelInitContext( struct intel_context *intel, 
				   const __GLcontextModes *mesaVis,
				   __DRIcontextPrivate *driContextPriv,
				   void *sharedContextPrivate,
				   struct dd_function_table *functions );

extern void intelGetLock(struct intel_context *intel, GLuint flags);

extern void intelInitState( GLcontext *ctx );
extern void intelFinish( GLcontext *ctx );
extern void intelFlush( GLcontext *ctx );

extern void intelInitDriverFunctions( struct dd_function_table *functions );


/* ================================================================
 * intel_state.c:
 */
extern void intelInitStateFuncs( struct dd_function_table *functions );

#define COMPAREFUNC_ALWAYS		0
#define COMPAREFUNC_NEVER		0x1
#define COMPAREFUNC_LESS		0x2
#define COMPAREFUNC_EQUAL		0x3
#define COMPAREFUNC_LEQUAL		0x4
#define COMPAREFUNC_GREATER		0x5
#define COMPAREFUNC_NOTEQUAL		0x6
#define COMPAREFUNC_GEQUAL		0x7

#define STENCILOP_KEEP			0
#define STENCILOP_ZERO			0x1
#define STENCILOP_REPLACE		0x2
#define STENCILOP_INCRSAT		0x3
#define STENCILOP_DECRSAT		0x4
#define STENCILOP_INCR			0x5
#define STENCILOP_DECR			0x6
#define STENCILOP_INVERT		0x7

#define LOGICOP_CLEAR			0
#define LOGICOP_NOR			0x1
#define LOGICOP_AND_INV 		0x2
#define LOGICOP_COPY_INV		0x3
#define LOGICOP_AND_RVRSE		0x4
#define LOGICOP_INV			0x5
#define LOGICOP_XOR			0x6
#define LOGICOP_NAND			0x7
#define LOGICOP_AND			0x8
#define LOGICOP_EQUIV			0x9
#define LOGICOP_NOOP			0xa
#define LOGICOP_OR_INV			0xb
#define LOGICOP_COPY			0xc
#define LOGICOP_OR_RVRSE		0xd
#define LOGICOP_OR			0xe
#define LOGICOP_SET			0xf

#define BLENDFACT_ZERO			0x01
#define BLENDFACT_ONE			0x02
#define BLENDFACT_SRC_COLR		0x03
#define BLENDFACT_INV_SRC_COLR 		0x04
#define BLENDFACT_SRC_ALPHA		0x05
#define BLENDFACT_INV_SRC_ALPHA 	0x06
#define BLENDFACT_DST_ALPHA		0x07
#define BLENDFACT_INV_DST_ALPHA 	0x08
#define BLENDFACT_DST_COLR		0x09
#define BLENDFACT_INV_DST_COLR		0x0a
#define BLENDFACT_SRC_ALPHA_SATURATE	0x0b
#define BLENDFACT_CONST_COLOR		0x0c
#define BLENDFACT_INV_CONST_COLOR	0x0d
#define BLENDFACT_CONST_ALPHA		0x0e
#define BLENDFACT_INV_CONST_ALPHA	0x0f
#define BLENDFACT_MASK          	0x0f

extern int intel_translate_shadow_compare_func( GLenum func );
extern int intel_translate_compare_func( GLenum func );
extern int intel_translate_stencil_op( GLenum op );
extern int intel_translate_blend_factor( GLenum factor );
extern int intel_translate_logic_op( GLenum opcode );


/* ================================================================
 * intel_buffers.c:
 */
void intelInitBufferFuncs( struct dd_function_table *functions );

struct intel_region *intel_readbuf_region( struct intel_context *intel );
struct intel_region *intel_drawbuf_region( struct intel_context *intel );

extern void intelWindowMoved( struct intel_context *intel );

extern GLboolean intel_intersect_cliprects( drm_clip_rect_t *dest,
					    const drm_clip_rect_t *a,
					    const drm_clip_rect_t *b );


/* ================================================================
 * intel_pixel_copy.c:
 */
void intelCopyPixels(GLcontext * ctx,
                     GLint srcx, GLint srcy,
                     GLsizei width, GLsizei height,
                     GLint destx, GLint desty, GLenum type);

GLboolean intel_check_blit_fragment_ops(GLcontext * ctx);

void intelBitmap(GLcontext * ctx,
		 GLint x, GLint y,
		 GLsizei width, GLsizei height,
		 const struct gl_pixelstore_attrib *unpack,
		 const GLubyte * pixels);

void intelInitExtensions(GLcontext *ctx, GLboolean enable_imaging);
#define _NEW_WINDOW_POS 0x40000000


/*======================================================================
 * Inline conversion functions.  
 * These are better-typed than the macros used previously:
 */
static inline struct intel_context *intel_context( GLcontext *ctx )
{
   return (struct intel_context *)ctx;
}

static inline struct intel_texture_object *intel_texture_object( struct gl_texture_object *obj )
{
   return (struct intel_texture_object *)obj;
}

static inline struct intel_texture_image *intel_texture_image( struct gl_texture_image *img )
{
   return (struct intel_texture_image *)img;
}

#endif

