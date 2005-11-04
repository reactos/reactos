/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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


#ifndef XMESAP_H
#define XMESAP_H


#ifdef XFree86Server
# include "GL/xf86glx.h"
# include "xf86glx_util.h"
#elif defined(USE_XSHM)
# include <X11/extensions/XShm.h>
#endif
#include "GL/xmesa.h"
#include "mtypes.h"
#if defined(FX)
#include "GL/fxmesa.h"
#include "../glide/fxdrv.h"
#endif


extern _glthread_Mutex _xmesa_lock;


/* for PF_8R8G8B24 pixel format */
typedef struct {
   GLubyte b;
   GLubyte g;
   GLubyte r;
} bgr_t;


/* Function pointer for clearing color buffers */
typedef void (*clear_func)( GLcontext *ctx,
                            GLboolean all, GLint x, GLint y,
                            GLint width, GLint height );




/** Framebuffer pixel formats */
enum pixel_format {
   PF_Index,		/**< Color Index mode */
   PF_Truecolor,	/**< TrueColor or DirectColor, any depth */
   PF_Dither_True,	/**< TrueColor with dithering */
   PF_8A8B8G8R,		/**< 32-bit TrueColor:  8-A, 8-B, 8-G, 8-R */
   PF_8R8G8B,		/**< 32-bit TrueColor:  8-R, 8-G, 8-B bits */
   PF_5R6G5B,		/**< 16-bit TrueColor:  5-R, 6-G, 5-B bits */
   PF_Dither,		/**< Color-mapped RGB with dither */
   PF_Lookup,		/**< Color-mapped RGB without dither */
   PF_HPCR,		/**< HP Color Recovery (ad@lms.be 30/08/95) */
   PF_1Bit,		/**< monochrome dithering of RGB */
   PF_Grayscale,	/**< Grayscale or StaticGray */
   PF_8R8G8B24,		/**< 24-bit TrueColor: 8-R, 8-G, 8-B bits */
   PF_Dither_5R6G5B	/**< 16-bit dithered TrueColor: 5-R, 6-G, 5-B */
};


/*
 * "Derived" from GLvisual.  Basically corresponds to an XVisualInfo.
 */
struct xmesa_visual {
   GLvisual mesa_visual;	/* Device independent visual parameters */
   XMesaDisplay *display;	/* The X11 display */
#ifdef XFree86Server
   GLint ColormapEntries;
   GLint nplanes;
#else
   XMesaVisualInfo visinfo;	/* X's visual info (pointer to private copy) */
   XVisualInfo *vishandle;	/* Only used in fakeglx.c */
#endif
   GLint BitsPerPixel;		/* True bits per pixel for XImages */

   GLboolean ximage_flag;	/* Use XImage for back buffer (not pixmap)? */

   enum pixel_format dithered_pf;  /* Pixel format when dithering */
   enum pixel_format undithered_pf;/* Pixel format when not dithering */

   GLfloat RedGamma;		/* Gamma values, 1.0 is default */
   GLfloat GreenGamma;
   GLfloat BlueGamma;

   /* For PF_TRUECOLOR */
   GLint rshift, gshift, bshift;/* Pixel color component shifts */
   GLubyte Kernel[16];		/* Dither kernel */
   unsigned long RtoPixel[512];	/* RGB to pixel conversion */
   unsigned long GtoPixel[512];
   unsigned long BtoPixel[512];
   GLubyte PixelToR[256];	/* Pixel to RGB conversion */
   GLubyte PixelToG[256];
   GLubyte PixelToB[256];

   /* For PF_HPCR */
   short       hpcr_rgbTbl[3][256];
   GLboolean   hpcr_clear_flag;
   GLubyte     hpcr_clear_ximage_pattern[2][16];
   XMesaImage *hpcr_clear_ximage;
   XMesaPixmap hpcr_clear_pixmap;

   /* For PF_1BIT */
   int bitFlip;
};


/*
 * "Derived" from __GLcontextRec.  Basically corresponds to a GLXContext.
 */
struct xmesa_context {
   GLcontext mesa;		/* the core library context (containment) */
   XMesaVisual xm_visual;	/* Describes the buffers */
   XMesaBuffer xm_draw_buffer;	/* current draw framebuffer */
   XMesaBuffer xm_read_buffer;	/* current read framebuffer */
   XMesaBuffer xm_buffer;	/* current span/point/line/triangle buffer */

   XMesaDisplay *display;	/* == xm_visual->display */
   GLboolean swapbytes;		/* Host byte order != display byte order? */
   GLboolean direct;		/* Direct rendering context? */

   enum pixel_format pixelformat;

   GLubyte clearcolor[4];		/* current clearing color */
   unsigned long clearpixel;		/* current clearing pixel value */
};



typedef enum {
   WINDOW,          /* An X window */
   GLXWINDOW,       /* GLX window */
   PIXMAP,          /* GLX pixmap */
   PBUFFER          /* GLX Pbuffer */
} BufferType;


/*
 * "Derived" from GLframebuffer.  Basically corresponds to a GLXDrawable.
 */
struct xmesa_buffer {
   GLframebuffer mesa_buffer;	/* depth, stencil, accum, etc buffers */
				/* This MUST BE FIRST! */
   GLboolean wasCurrent;	/* was ever the current buffer? */
   XMesaVisual xm_visual;	/* the X/Mesa visual */

   XMesaDisplay *display;
   BufferType type;             /* window, pixmap, pbuffer or glxwindow */
   XMesaDrawable frontbuffer;	/* either a window or pixmap */
   XMesaPixmap backpixmap;	/* back buffer Pixmap */
   XMesaImage *backimage;	/* back buffer simulated XImage */

   XMesaDrawable buffer;	/* the current buffer, either equal to */
				/* frontbuffer, backpixmap or XIMAGE (None) */

   XMesaColormap cmap;		/* the X colormap */

   unsigned long selectedEvents;/* for pbuffers only */

   GLint db_state;		/* 0 = single buffered */
				/* BACK_PIXMAP = use Pixmap for back buffer */
				/* BACK_XIMAGE = use XImage for back buffer */

#ifndef XFree86Server
   GLuint shm;			/* X Shared Memory extension status:	*/
				/*    0 = not available			*/
				/*    1 = XImage support available	*/
				/*    2 = Pixmap support available too	*/
#ifdef USE_XSHM
   XShmSegmentInfo shminfo;
#endif
#endif

   XMesaImage *rowimage;	/* Used for optimized span writing */

   GLuint width, height;	/* size of buffer */

   GLint bottom;		/* used for FLIP macro below */
   GLubyte *ximage_origin1;	/* used for PIXELADDR1 macro */
   GLint ximage_width1;
   GLushort *ximage_origin2;	/* used for PIXELADDR2 macro */
   GLint ximage_width2;
   GLubyte *ximage_origin3;	/* used for PIXELADDR3 macro */
   GLint ximage_width3;
   GLuint *ximage_origin4;	/* used for PIXELADDR4 macro */
   GLint ximage_width4;

   XMesaPixmap stipple_pixmap;	/* For polygon stippling */
   XMesaGC stipple_gc;		/* For polygon stippling */

   XMesaGC gc;			/* scratch GC for span, line, tri drawing */
   XMesaGC cleargc;		/* GC for clearing the color buffer */
   XMesaGC swapgc;		/* GC for swapping the color buffers */

   /* The following are here instead of in the XMesaVisual
    * because they depend on the window's colormap.
    */

   /* For PF_DITHER, PF_LOOKUP, PF_GRAYSCALE */
   unsigned long color_table[576];	/* RGB -> pixel value */

   /* For PF_DITHER, PF_LOOKUP, PF_GRAYSCALE */
   GLubyte pixel_to_r[65536];		/* pixel value -> red */
   GLubyte pixel_to_g[65536];		/* pixel value -> green */
   GLubyte pixel_to_b[65536];		/* pixel value -> blue */

   /* Used to do XAllocColor/XFreeColors accounting: */
   int num_alloced;
#if defined(XFree86Server)
   Pixel alloced_colors[256];
#else
   unsigned long alloced_colors[256];
#endif

#if defined( FX )
   /* For 3Dfx Glide only */
   GLboolean FXisHackUsable;	/* Can we render into window? */
   GLboolean FXwindowHack;	/* Are we rendering into a window? */
   fxMesaContext FXctx;
#endif

   /* functions for clearing the front and back color buffers */
   clear_func front_clear_func;
   clear_func back_clear_func;

   struct xmesa_buffer *Next;	/* Linked list pointer: */
};



/* Values for xmesa->dest: */
#define FRONT_PIXMAP	1
#define BACK_PIXMAP	2
#define BACK_XIMAGE	4

/* Special value for X Drawable variables to indicate use of XImage instead */
#define XIMAGE None


/*
 * If pixelformat==PF_TRUECOLOR:
 */
#define PACK_TRUECOLOR( PIXEL, R, G, B )	\
   PIXEL = xmesa->xm_visual->RtoPixel[R]	\
         | xmesa->xm_visual->GtoPixel[G]	\
         | xmesa->xm_visual->BtoPixel[B];	\


/*
 * If pixelformat==PF_TRUEDITHER:
 */
#define PACK_TRUEDITHER( PIXEL, X, Y, R, G, B )			\
{								\
   int d = xmesa->xm_visual->Kernel[((X)&3) | (((Y)&3)<<2)];	\
   PIXEL = xmesa->xm_visual->RtoPixel[(R)+d]			\
         | xmesa->xm_visual->GtoPixel[(G)+d]			\
         | xmesa->xm_visual->BtoPixel[(B)+d];			\
}



/*
 * If pixelformat==PF_8A8B8G8R:
 */
#define PACK_8A8B8G8R( R, G, B, A )	\
	( ((A) << 24) | ((B) << 16) | ((G) << 8) | (R) )


/*
 * Like PACK_8A8B8G8R() but don't use alpha.  This is usually an acceptable
 * shortcut.
 */
#define PACK_8B8G8R( R, G, B )   ( ((B) << 16) | ((G) << 8) | (R) )



/*
 * If pixelformat==PF_8R8G8B:
 */
#define PACK_8R8G8B( R, G, B)	 ( ((R) << 16) | ((G) << 8) | (B) )


/*
 * If pixelformat==PF_5R6G5B:
 */
#define PACK_5R6G5B( R, G, B)	 ( (((R) & 0xf8) << 8) | (((G) & 0xfc) << 3) | ((B) >> 3) )




/*
 * If pixelformat==PF_DITHER:
 *
 * Improved 8-bit RGB dithering code contributed by Bob Mercier
 * (mercier@hollywood.cinenet.net).  Thanks Bob!
 */
#ifdef DITHER666
# define DITH_R   6
# define DITH_G   6
# define DITH_B   6
# define DITH_MIX(r,g,b)  (((r) * DITH_G + (g)) * DITH_B + (b))
#else
# define DITH_R	5
# define DITH_G	9
# define DITH_B	5
# define DITH_MIX(r,g,b)  (((g) << 6) | ((b) << 3) | (r))
#endif
#define DITH_DX	4
#define DITH_DY	4
#define DITH_N	(DITH_DX * DITH_DY)

#define _dither(C, c, d)   (((unsigned)((DITH_N * (C - 1) + 1) * c + d)) >> 12)

#define MAXC	256
extern const int xmesa_kernel8[DITH_DY * DITH_DX];

/* Dither for random X,Y */
#define DITHER_SETUP						\
	int __d;						\
	unsigned long *ctable = xmesa->xm_buffer->color_table;

#define DITHER( X, Y, R, G, B )				\
	(__d = xmesa_kernel8[(((Y)&3)<<2) | ((X)&3)],	\
	 ctable[DITH_MIX(_dither(DITH_R, (R), __d),	\
		         _dither(DITH_G, (G), __d),	\
		         _dither(DITH_B, (B), __d))])

/* Dither for random X, fixed Y */
#define XDITHER_SETUP(Y)					\
	int __d;						\
	unsigned long *ctable = xmesa->xm_buffer->color_table;	\
	const int *kernel = &xmesa_kernel8[ ((Y)&3) << 2 ];

#define XDITHER( X, R, G, B )				\
	(__d = kernel[(X)&3],				\
	ctable[DITH_MIX(_dither(DITH_R, (R), __d),	\
		        _dither(DITH_G, (G), __d),	\
		        _dither(DITH_B, (B), __d))])



/*
 * Dithering for flat-shaded triangles.  Precompute all 16 possible
 * pixel values given the triangle's RGB color.  Contributed by Martin Shenk.
 */
#define FLAT_DITHER_SETUP( R, G, B )					\
	GLushort ditherValues[16];					\
	{								\
	   unsigned long *ctable = xmesa->xm_buffer->color_table;	\
	   int msdr = (DITH_N*((DITH_R)-1)+1) * (R);			\
	   int msdg = (DITH_N*((DITH_G)-1)+1) * (G);			\
	   int msdb = (DITH_N*((DITH_B)-1)+1) * (B);			\
	   int i;							\
	   for (i=0;i<16;i++) {						\
	      int k = xmesa_kernel8[i];					\
	      int j = DITH_MIX( (msdr+k)>>12, (msdg+k)>>12, (msdb+k)>>12 );\
	      ditherValues[i] = (GLushort) ctable[j];			\
	   }								\
        }

#define FLAT_DITHER_ROW_SETUP(Y)					\
	GLushort *ditherRow = ditherValues + ( ((Y)&3) << 2);

#define FLAT_DITHER(X)  ditherRow[(X)&3]



/*
 * If pixelformat==PF_LOOKUP:
 */
#define _dither_lookup(C, c)   (((unsigned)((DITH_N * (C - 1) + 1) * c)) >> 12)

#define LOOKUP_SETUP						\
	unsigned long *ctable = xmesa->xm_buffer->color_table

#define LOOKUP( R, G, B )				\
	ctable[DITH_MIX(_dither_lookup(DITH_R, (R)),	\
		        _dither_lookup(DITH_G, (G)),	\
		        _dither_lookup(DITH_B, (B)))]



/*
 * If pixelformat==PF_HPCR:
 *
 *      HP Color Recovery dithering               (ad@lms.be 30/08/95)
 *      HP has on it's 8-bit 700-series computers, a feature called
 *      'Color Recovery'.  This allows near 24-bit output (so they say).
 *      It is enabled by selecting the 8-bit  TrueColor  visual AND
 *      corresponding  colormap (see tkInitWindow) AND doing some special
 *      dither.
 */
extern const short xmesa_HPCR_DRGB[3][2][16];

#define DITHER_HPCR( X, Y, R, G, B )					   \
  ( ((xmesa->xm_visual->hpcr_rgbTbl[0][R] + xmesa_HPCR_DRGB[0][(Y)&1][(X)&15]) & 0xE0)     \
  |(((xmesa->xm_visual->hpcr_rgbTbl[1][G] + xmesa_HPCR_DRGB[1][(Y)&1][(X)&15]) & 0xE0)>>3) \
  | ((xmesa->xm_visual->hpcr_rgbTbl[2][B] + xmesa_HPCR_DRGB[2][(Y)&1][(X)&15])>>6)	   \
  )



/*
 * If pixelformat==PF_1BIT:
 */
extern const int xmesa_kernel1[16];

#define SETUP_1BIT  int bitFlip = xmesa->xm_visual->bitFlip
#define DITHER_1BIT( X, Y, R, G, B )	\
	(( ((int)(R)+(int)(G)+(int)(B)) > xmesa_kernel1[(((Y)&3) << 2) | ((X)&3)] ) ^ bitFlip)



/*
 * If pixelformat==PF_GRAYSCALE:
 */
#define GRAY_RGB( R, G, B )   xmesa->xm_buffer->color_table[((R) + (G) + (B))/3]



/*
 * Converts a GL window Y coord to an X window Y coord:
 */
#define FLIP(BUFFER, Y)  ((BUFFER)->bottom-(Y))


/*
 * Return the address of a 1, 2 or 4-byte pixel in the back XImage:
 * X==0 is left, Y==0 is bottom.
 */
#define PIXELADDR1( BUFFER, X, Y )  \
   ( (BUFFER)->ximage_origin1 - (Y) * (BUFFER)->ximage_width1 + (X) )

#define PIXELADDR2( BUFFER, X, Y )  \
   ( (BUFFER)->ximage_origin2 - (Y) * (BUFFER)->ximage_width2 + (X) )

#define PIXELADDR3( BUFFER, X, Y )  \
   ( (bgr_t *) ( (BUFFER)->ximage_origin3 - (Y) * (BUFFER)->ximage_width3 + 3 * (X) ))

#define PIXELADDR4( BUFFER, X, Y )  \
   ( (BUFFER)->ximage_origin4 - (Y) * (BUFFER)->ximage_width4 + (X) )




/*
 * Return pointer to XMesaContext corresponding to a Mesa GLcontext.
 * Since we're using structure containment, it's just a cast!.
 */
#define XMESA_CONTEXT(MESACTX)  (XMesaContext) (MESACTX)



/*
 * External functions:
 */

extern unsigned long
xmesa_color_to_pixel( XMesaContext xmesa,
                      GLubyte r, GLubyte g, GLubyte b, GLubyte a,
                      GLuint pixelFormat );

extern void xmesa_alloc_back_buffer( XMesaBuffer b );

extern void xmesa_resize_buffers( GLframebuffer *buffer );

extern void xmesa_init_driver_functions( XMesaVisual xmvisual,
                                         struct dd_function_table *driver );

extern void xmesa_update_state( GLcontext *ctx, GLuint new_state );

extern void xmesa_update_span_funcs( GLcontext *ctx );

extern void xmesa_set_buffer( GLcontext *ctx, GLframebuffer *buffer,
                              GLuint bufferBit );


/* Plugged into the software rasterizer.  Try to use internal
 * swrast-style point, line and triangle functions.
 */
extern void xmesa_choose_point( GLcontext *ctx );
extern void xmesa_choose_line( GLcontext *ctx );
extern void xmesa_choose_triangle( GLcontext *ctx );


extern void xmesa_register_swrast_functions( GLcontext *ctx );



/* XXX this is a hack to implement shared display lists with 3Dfx */
extern XMesaBuffer XMesaCreateWindowBuffer2( XMesaVisual v,
					     XMesaWindow w,
					     XMesaContext c );

/*
 * These are the extra routines required for integration with XFree86.
 * None of these routines should be user visible. -KEM
 */
extern void XMesaSetVisualDisplay( XMesaDisplay *dpy, XMesaVisual v );
extern GLboolean XMesaForceCurrent(XMesaContext c);
extern GLboolean XMesaLoseCurrent(XMesaContext c);
extern void XMesaReset( void );


#endif
