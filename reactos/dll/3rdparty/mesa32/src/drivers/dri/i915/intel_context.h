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
#include "mm.h"
#include "texmem.h"
#include "vblank.h"

#include "intel_screen.h"
#include "i915_drm.h"
#include "i830_common.h"
#include "tnl/t_vertex.h"

#define TAG(x) intel##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

#define DV_PF_555  (1<<8)
#define DV_PF_565  (2<<8)
#define DV_PF_8888 (3<<8)

#define INTEL_CONTEXT(ctx)	((intelContextPtr)(ctx))

typedef struct intel_context intelContext;
typedef struct intel_context *intelContextPtr;
typedef struct intel_texture_object *intelTextureObjectPtr;

typedef void (*intel_tri_func)(intelContextPtr, intelVertex *, intelVertex *,
							  intelVertex *);
typedef void (*intel_line_func)(intelContextPtr, intelVertex *, intelVertex *);
typedef void (*intel_point_func)(intelContextPtr, intelVertex *);

#define INTEL_FALLBACK_DRAW_BUFFER	 0x1
#define INTEL_FALLBACK_READ_BUFFER	 0x2
#define INTEL_FALLBACK_USER		 0x4
#define INTEL_FALLBACK_NO_BATCHBUFFER	 0x8
#define INTEL_FALLBACK_NO_TEXMEM	 0x10
#define INTEL_FALLBACK_RENDERMODE	 0x20

extern void intelFallback( intelContextPtr intel, GLuint bit, GLboolean mode );
#define FALLBACK( intel, bit, mode ) intelFallback( intel, bit, mode )


#define INTEL_TEX_MAXLEVELS 10


struct intel_texture_object
{
   driTextureObject    base;	/* the parent class */

   GLuint texelBytes;
   GLuint age;
   GLuint Pitch;
   GLuint Height;
   GLuint TextureOffset;
   GLubyte *BufAddr;   

   GLuint min_level;
   GLuint max_level;
   GLuint depth_pitch;

   struct {
      const struct gl_texture_image *image;
      GLuint offset;       /* into BufAddr */
      GLuint height;
      GLuint internalFormat;
   } image[6][INTEL_TEX_MAXLEVELS];

   GLuint dirty;
   GLuint firstLevel,lastLevel;
};


struct intel_context
{
   GLcontext ctx;		/* the parent class */

   struct {
      void (*destroy)( intelContextPtr intel ); 
      void (*emit_state)( intelContextPtr intel );
      void (*lost_hardware)( intelContextPtr intel );
      void (*update_texture_state)( intelContextPtr intel );

      void (*render_start)( intelContextPtr intel );
      void (*set_color_region)( intelContextPtr intel, const intelRegion *reg );
      void (*set_z_region)( intelContextPtr intel, const intelRegion *reg );
      void (*update_color_z_regions)(intelContextPtr intel,
                                     const intelRegion *colorRegion,
                                     const intelRegion *depthRegion);
      void (*emit_flush)( intelContextPtr intel );
      void (*reduced_primitive_state)( intelContextPtr intel, GLenum rprim );

      GLboolean (*check_vertex_size)( intelContextPtr intel, GLuint expected );

      void (*clear_with_tris)( intelContextPtr intel, GLbitfield mask,
			       GLboolean all, 
			       GLint cx, GLint cy, GLint cw, GLint ch);

      void (*rotate_window)( intelContextPtr intel,
                             __DRIdrawablePrivate *dPriv, GLuint srcBuf);

      intelTextureObjectPtr (*alloc_tex_obj)( struct gl_texture_object *tObj );

   } vtbl;

   GLint refcount;   
   GLuint Fallback;
   GLuint NewGLState;
   
   struct {
      GLuint start_offset;
      GLint size;
      GLint space;
      GLubyte *ptr;
      GLuint counter;
      GLuint last_emit_state;
      GLboolean contains_geometry;
      const char *func;
      GLuint last_swap;
   } batch;
      
   struct {
      void *ptr;
      GLint size;
      GLuint offset;
      GLuint active_buf;
      GLuint irq_emitted;
   } alloc;

   struct {
      GLuint primitive;
      GLubyte *start_ptr;      
      void (*flush)( GLcontext * );
   } prim;

   GLboolean locked;

   GLubyte clear_red;
   GLubyte clear_green;
   GLubyte clear_blue;
   GLubyte clear_alpha;
   GLuint ClearColor;
   GLuint ClearDepth;

   GLuint coloroffset;
   GLuint specoffset;

   /* Support for duplicating XYZW as WPOS parameter (crutch for I915).
    */
   GLuint wpos_offset;
   GLuint wpos_size;

   struct tnl_attr_map vertex_attrs[VERT_ATTRIB_MAX];
   GLuint vertex_attr_count;

   GLfloat depth_scale;
   GLfloat polygon_offset_scale; /* dependent on depth_scale, bpp */
   GLuint depth_clear_mask;
   GLuint stencil_clear_mask;

   GLboolean hw_stencil;
   GLboolean hw_stipple;
   
   /* Texture object bookkeeping
    */
   GLuint                nr_heaps;
   driTexHeap          * texture_heaps[1];
   driTextureObject      swapped;
   GLuint                lastStamp;

   struct intel_texture_object *CurrentTexObj[MAX_TEXTURE_UNITS];

   /* State for intelvb.c and inteltris.c.
    */
   GLuint RenderIndex;
   GLmatrix ViewportMatrix;
   GLenum render_primitive;
   GLenum reduced_primitive;
   GLuint vertex_size;
   unsigned char *verts;	   /* points to tnl->clipspace.vertex_buf */


   /* Fallback rasterization functions 
    */
   intel_point_func draw_point;
   intel_line_func draw_line;
   intel_tri_func draw_tri;

   /* Drawing buffer state
    */
   intelRegion *drawRegion;  /* current drawing buffer */
   intelRegion *readRegion;  /* current reading buffer */

   int drawX;			/* origin of drawable in draw buffer */
   int drawY;
   GLuint numClipRects;		/* cliprects for that buffer */
   drm_clip_rect_t *pClipRects;

   int dirtyAge;
   int perf_boxes;

   GLuint do_usleeps;
   int do_irqs;
   GLuint irqsEmitted;
   drm_i915_irq_wait_t iw;

   GLboolean scissor;
   drm_clip_rect_t draw_rect;
   drm_clip_rect_t scissor_rect;

   drm_context_t hHWContext;
   drmLock *driHwLock;
   int driFd;

   __DRIdrawablePrivate *driDrawable;
   __DRIscreenPrivate *driScreen;
   intelScreenPrivate *intelScreen; 
   drmI830Sarea *sarea; 

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

   GLuint swap_scheduled;
};


#define DEBUG_LOCKING	1

#if DEBUG_LOCKING
extern char *prevLockFile;
extern int prevLockLine;

#define DEBUG_LOCK()							\
   do {									\
      prevLockFile = (__FILE__);					\
      prevLockLine = (__LINE__);					\
   } while (0)

#define DEBUG_RESET()							\
   do {									\
      prevLockFile = 0;							\
      prevLockLine = 0;							\
   } while (0)

/* Slightly less broken way of detecting recursive locking in a
 * threaded environment.  The right way to do this would be to make
 * prevLockFile, prevLockLine thread-local.
 *
 * This technique instead checks to see if the same context is
 * requesting the lock twice -- this will not catch application
 * breakages where the same context is active in two different threads
 * at once, but it will catch driver breakages (recursive locking) in
 * threaded apps.
 */
#define DEBUG_CHECK_LOCK()						\
   do {									\
      if ( *((volatile int *)intel->driHwLock) == 			\
	   (DRM_LOCK_HELD | intel->hHWContext) ) {			\
	 fprintf( stderr,						\
		  "LOCK SET!\n\tPrevious %s:%d\n\tCurrent: %s:%d\n",	\
		  prevLockFile, prevLockLine, __FILE__, __LINE__ );	\
	 abort();							\
      }									\
   } while (0)

#else

#define DEBUG_LOCK()
#define DEBUG_RESET()
#define DEBUG_CHECK_LOCK()

#endif




/* Lock the hardware and validate our state.  
 */
#define LOCK_HARDWARE( intel )				\
do {							\
    char __ret=0;					\
    DEBUG_CHECK_LOCK();					\
    assert(!(intel)->locked);				\
    if ((intel)->swap_scheduled) {			\
        drmVBlank vbl;					\
        vbl.request.type = DRM_VBLANK_ABSOLUTE;		\
        if ((intel)->vblank_flags &			\
            VBLANK_FLAG_SECONDARY) {			\
            vbl.request.type |= DRM_VBLANK_SECONDARY;	\
        }						\
        vbl.request.sequence = (intel)->vbl_seq;	\
        drmWaitVBlank((intel)->driFd, &vbl);		\
        (intel)->swap_scheduled = 0;			\
    }							\
    DRM_CAS((intel)->driHwLock, (intel)->hHWContext,	\
        (DRM_LOCK_HELD|(intel)->hHWContext), __ret);	\
    if (__ret)						\
        intelGetLock( (intel), 0 );			\
      DEBUG_LOCK();					\
    (intel)->locked = 1;				\
}while (0)
 
  
  /* Unlock the hardware using the global current context 
   */
#define UNLOCK_HARDWARE(intel)						\
do {									\
   intel->locked = 0;							\
   if (0) { 								\
      intel->perf_boxes |= intel->sarea->perf_boxes;  			\
      intel->sarea->perf_boxes = 0;					\
   }									\
   DRM_UNLOCK((intel)->driFd, (intel)->driHwLock, (intel)->hHWContext);	\
   DEBUG_RESET();							\
} while (0)


#define SUBPIXEL_X 0.125
#define SUBPIXEL_Y 0.125

#define INTEL_FIREVERTICES(intel)		\
do {						\
   if ((intel)->prim.flush)			\
      (intel)->prim.flush(&(intel)->ctx);		\
} while (0)

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
static __inline__ void * __memcpy(void * to, const void * from, size_t n)
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



/* ================================================================
 * Debugging:
 */
#define DO_DEBUG		1
#if DO_DEBUG
extern int INTEL_DEBUG;
#else
#define INTEL_DEBUG		0
#endif

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


#define PCI_CHIP_845_G			0x2562
#define PCI_CHIP_I830_M			0x3577
#define PCI_CHIP_I855_GM		0x3582
#define PCI_CHIP_I865_G			0x2572
#define PCI_CHIP_I915_G			0x2582
#define PCI_CHIP_I915_GM		0x2592
#define PCI_CHIP_I945_G			0x2772
#define PCI_CHIP_I945_GM		0x27A2


/* ================================================================
 * intel_context.c:
 */

extern void intelInitDriverFunctions( struct dd_function_table *functions );

extern GLboolean intelInitContext( intelContextPtr intel, 
				   const __GLcontextModes *mesaVis,
				   __DRIcontextPrivate *driContextPriv,
				   void *sharedContextPrivate,
				   struct dd_function_table *functions );

extern void intelGetLock(intelContextPtr intel, GLuint flags);
extern void intelSetBackClipRects(intelContextPtr intel);
extern void intelSetFrontClipRects(intelContextPtr intel);
extern void intelWindowMoved( intelContextPtr intel );

extern void intelInitState( GLcontext *ctx );
extern const GLubyte *intelGetString( GLcontext *ctx, GLenum name );


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


extern int intel_translate_compare_func( GLenum func );
extern int intel_translate_stencil_op( GLenum op );
extern int intel_translate_blend_factor( GLenum factor );
extern int intel_translate_logic_op( GLenum opcode );


/* ================================================================
 * intel_ioctl.c:
 */
extern void intel_dump_batchbuffer( long offset,
				    int *ptr,
				    int count );


/* ================================================================
 * intel_pixel.c:
 */	
extern void intelInitPixelFuncs( struct dd_function_table *functions );



#endif

