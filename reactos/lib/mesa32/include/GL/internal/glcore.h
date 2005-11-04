/* $XFree86: xc/lib/GL/include/GL/internal/glcore.h,v 1.7 2001/03/25 05:32:00 tsi Exp $ */
#ifndef __gl_core_h_
#define __gl_core_h_

/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
** 
** http://oss.sgi.com/projects/FreeB
** 
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
** 
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
** 
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
**
*/

#ifndef XFree86LOADER
#include <sys/types.h>
#endif

#ifdef CAPI
#undef CAPI
#endif
#define CAPI

#define GL_CORE_SGI  1
#define GL_CORE_MESA 2
#define GL_CORE_APPLE 4

typedef struct __GLcontextRec __GLcontext;
typedef struct __GLinterfaceRec __GLinterface;

/*
** This file defines the interface between the GL core and the surrounding
** "operating system" that supports it (currently the GLX or WGL extensions).
**
** Members (data and function pointers) are documented as imported or
** exported according to how they are used by the core rendering functions.
** Imported members are initialized by the "operating system" and used by
** the core functions.  Exported members are initialized by the core functions
** and used by the "operating system".
*/

/*
** Mode and limit information for a context.  This information is
** kept around in the context so that values can be used during
** command execution, and for returning information about the
** context to the application.
*/
typedef struct __GLcontextModesRec {
    struct __GLcontextModesRec * next;

    GLboolean rgbMode;
    GLboolean floatMode;
    GLboolean colorIndexMode;
    GLuint doubleBufferMode;
    GLuint stereoMode;

    GLboolean haveAccumBuffer;
    GLboolean haveDepthBuffer;
    GLboolean haveStencilBuffer;

    GLint redBits, greenBits, blueBits, alphaBits;	/* bits per comp */
    GLuint redMask, greenMask, blueMask, alphaMask;
    GLint rgbBits;		/* total bits for rgb */
    GLint indexBits;		/* total bits for colorindex */

    GLint accumRedBits, accumGreenBits, accumBlueBits, accumAlphaBits;
    GLint depthBits;
    GLint stencilBits;

    GLint numAuxBuffers;

    GLint level;

    GLint pixmapMode;

    /* GLX */
    GLint visualID;
    GLint visualType;     /**< One of the GLX X visual types. (i.e., 
			   * \c GLX_TRUE_COLOR, etc.)
			   */

    /* EXT_visual_rating / GLX 1.2 */
    GLint visualRating;

    /* EXT_visual_info / GLX 1.2 */
    GLint transparentPixel;
				/*    colors are floats scaled to ints */
    GLint transparentRed, transparentGreen, transparentBlue, transparentAlpha;
    GLint transparentIndex;

    /* ARB_multisample / SGIS_multisample */
    GLint sampleBuffers;
    GLint samples;

    /* SGIX_fbconfig / GLX 1.3 */
    GLint drawableType;
    GLint renderType;
    GLint xRenderable;
    GLint fbconfigID;

    /* SGIX_pbuffer / GLX 1.3 */
    GLint maxPbufferWidth;
    GLint maxPbufferHeight;
    GLint maxPbufferPixels;
    GLint optimalPbufferWidth;   /* Only for SGIX_pbuffer. */
    GLint optimalPbufferHeight;  /* Only for SGIX_pbuffer. */

    /* SGIX_visual_select_group */
    GLint visualSelectGroup;

    /* OML_swap_method */
    GLint swapMethod;

    GLint screen;
} __GLcontextModes;

/* Several fields of __GLcontextModes can take these as values.  Since
 * GLX header files may not be available everywhere they need to be used,
 * redefine them here.
 */
#define GLX_NONE                           0x8000
#define GLX_SLOW_CONFIG                    0x8001
#define GLX_TRUE_COLOR                     0x8002
#define GLX_DIRECT_COLOR                   0x8003
#define GLX_PSEUDO_COLOR                   0x8004
#define GLX_STATIC_COLOR                   0x8005
#define GLX_GRAY_SCALE                     0x8006
#define GLX_STATIC_GRAY                    0x8007
#define GLX_TRANSPARENT_RGB                0x8008
#define GLX_TRANSPARENT_INDEX              0x8009
#define GLX_NON_CONFORMANT_CONFIG          0x800D
#define GLX_SWAP_EXCHANGE_OML              0x8061
#define GLX_SWAP_COPY_OML                  0x8062
#define GLX_SWAP_UNDEFINED_OML             0x8063

#define GLX_DONT_CARE                      0xFFFFFFFF

#define GLX_RGBA_BIT                       0x00000001
#define GLX_COLOR_INDEX_BIT                0x00000002
#define GLX_WINDOW_BIT                     0x00000001
#define GLX_PIXMAP_BIT                     0x00000002
#define GLX_PBUFFER_BIT                    0x00000004

/************************************************************************/

/*
** Structure used for allocating and freeing drawable private memory.
** (like software buffers, for example).
**
** The memory allocation routines are provided by the surrounding
** "operating system" code, and they are to be used for allocating
** software buffers and things which are associated with the drawable,
** and used by any context which draws to that drawable.  There are
** separate memory allocation functions for drawables and contexts
** since drawables and contexts can be created and destroyed independently
** of one another, and the "operating system" may want to use separate
** allocation arenas for each.
**
** The freePrivate function is filled in by the core routines when they
** allocates software buffers, and stick them in "private".  The freePrivate
** function will destroy anything allocated to this drawable (to be called
** when the drawable is destroyed).
*/
typedef struct __GLdrawableRegionRec __GLdrawableRegion;
typedef struct __GLdrawableBufferRec __GLdrawableBuffer;
typedef struct __GLdrawablePrivateRec __GLdrawablePrivate;

typedef struct __GLregionRectRec {
    /* lower left (inside the rectangle) */
    GLint x0, y0;
    /* upper right (outside the rectangle) */
    GLint x1, y1;
} __GLregionRect;

struct __GLdrawableRegionRec {
    GLint numRects;
    __GLregionRect *rects;
    __GLregionRect boundingRect;
};

/************************************************************************/

/* masks for the buffers */
#define __GL_FRONT_BUFFER_MASK		0x00000001
#define	__GL_FRONT_LEFT_BUFFER_MASK	0x00000001
#define	__GL_FRONT_RIGHT_BUFFER_MASK	0x00000002
#define	__GL_BACK_BUFFER_MASK		0x00000004
#define __GL_BACK_LEFT_BUFFER_MASK	0x00000004
#define __GL_BACK_RIGHT_BUFFER_MASK	0x00000008
#define	__GL_ACCUM_BUFFER_MASK		0x00000010
#define	__GL_DEPTH_BUFFER_MASK		0x00000020
#define	__GL_STENCIL_BUFFER_MASK	0x00000040
#define	__GL_AUX_BUFFER_MASK(i)		(0x0000080 << (i))

#define __GL_ALL_BUFFER_MASK		0xffffffff

/* what Resize routines return if resize resorted to fallback case */
#define __GL_BUFFER_FALLBACK	0x10

typedef void (*__GLbufFallbackInitFn)(__GLdrawableBuffer *buf, 
				      __GLdrawablePrivate *glPriv, GLint bits);
typedef void (*__GLbufMainInitFn)(__GLdrawableBuffer *buf, 
				  __GLdrawablePrivate *glPriv, GLint bits,
				  __GLbufFallbackInitFn back);

/*
** A drawable buffer
**
** This data structure describes the context side of a drawable.  
**
** According to the spec there could be multiple contexts bound to the same
** drawable at the same time (from different threads).  In order to avoid
** multiple-access conflicts, locks are used to serialize access.  When a
** thread needs to access (read or write) a member of the drawable, it takes
** a lock first.  Some of the entries in the drawable are treated "mostly
** constant", so we take the freedom of allowing access to them without
** taking a lock (for optimization reasons).
**
** For more details regarding locking, see buffers.h in the GL core
*/
struct __GLdrawableBufferRec {
    /*
    ** Buffer dimensions
    */
    GLint width, height, depth;

    /*
    ** Framebuffer base address
    */
    void *base;

    /*
    ** Framebuffer size (in bytes)
    */
    GLuint size;

    /*
    ** Size (in bytes) of each element in the framebuffer
    */
    GLuint elementSize;
    GLuint elementSizeLog2;

    /*
    ** Element skip from one scanline to the next.
    ** If the buffer is part of another buffer (for example, fullscreen
    ** front buffer), outerWidth is the width of that buffer.
    */
    GLint outerWidth;

    /*
    ** outerWidth * elementSize
    */
    GLint byteWidth;

    /*
    ** Allocation/deallocation is done based on this handle.  A handle
    ** is conceptually different from the framebuffer 'base'.
    */
    void *handle;

    /* imported */
    GLboolean (*resize)(__GLdrawableBuffer *buf,
			GLint x, GLint y, GLuint width, GLuint height, 
			__GLdrawablePrivate *glPriv, GLuint bufferMask);
    void (*lock)(__GLdrawableBuffer *buf, __GLdrawablePrivate *glPriv);
    void (*unlock)(__GLdrawableBuffer *buf, __GLdrawablePrivate *glPriv);
    void (*fill)(__GLdrawableBuffer *buf, __GLdrawablePrivate *glPriv,
    		GLuint val, GLint x, GLint y, GLint w, GLint h);
    void (*free)(__GLdrawableBuffer *buf, __GLdrawablePrivate *glPriv);

    /* exported */
    void (*freePrivate)(__GLdrawableBuffer *buf, __GLdrawablePrivate *glPriv);
#ifdef __cplusplus
    void *privatePtr;
#else
    void *private;
#endif

    /* private */
    void *other;	/* implementation private data */
    __GLbufMainInitFn mainInit;
    __GLbufFallbackInitFn fallbackInit;
};

/*
** The context side of the drawable private
*/
struct __GLdrawablePrivateRec {
    /*
    ** Drawable Modes
    */
    __GLcontextModes *modes;

    /*
    ** Drawable size
    */
    GLuint width, height;

    /*
    ** Origin in screen coordinates of the drawable
    */
    GLint xOrigin, yOrigin;
#ifdef __GL_ALIGNED_BUFFERS
    /*
    ** Drawable offset from screen origin
    */
    GLint xOffset, yOffset;

    /*
    ** Alignment restriction
    */
    GLint xAlignment, yAlignment;
#endif
    /*
    ** Should we invert the y axis?
    */
    GLint yInverted;

    /*
    ** Mask specifying which buffers are renderable by the hw
    */
    GLuint accelBufferMask;

    /*
    ** the buffers themselves
    */
    __GLdrawableBuffer frontBuffer;
    __GLdrawableBuffer backBuffer;
    __GLdrawableBuffer accumBuffer;
    __GLdrawableBuffer depthBuffer;
    __GLdrawableBuffer stencilBuffer;
#if defined(__GL_NUMBER_OF_AUX_BUFFERS) && (__GL_NUMBER_OF_AUX_BUFFERS > 0)
    __GLdrawableBuffer *auxBuffer;
#endif

    __GLdrawableRegion ownershipRegion;

    /*
    ** Lock for the drawable private structure
    */
    void *lock;
#ifdef DEBUG
    /* lock debugging info */
    int lockRefCount;
    int lockLine[10];
    char *lockFile[10];
#endif

    /* imported */
    void *(*malloc)(size_t size);
    void *(*calloc)(size_t numElem, size_t elemSize);
    void *(*realloc)(void *oldAddr, size_t newSize);
    void (*free)(void *addr);

    GLboolean (*addSwapRect)(__GLdrawablePrivate *glPriv, 
			     GLint x, GLint y, GLsizei width, GLsizei height);
    void (*setClipRect)(__GLdrawablePrivate *glPriv, 
			GLint x, GLint y, GLsizei width, GLsizei height);
    void (*updateClipRegion)(__GLdrawablePrivate *glPriv);
    GLboolean (*resize)(__GLdrawablePrivate *glPriv);
    void (*getDrawableSize)(__GLdrawablePrivate *glPriv, 
			    GLint *x, GLint *y, GLuint *width, GLuint *height);

    void (*lockDP)(__GLdrawablePrivate *glPriv, __GLcontext *gc);
    void (*unlockDP)(__GLdrawablePrivate *glPriv);

    /* exported */
#ifdef __cplusplus
    void *privatePtr;
#else
    void *private;
#endif
    void (*freePrivate)(__GLdrawablePrivate *);

    /* client data */
    void *other;
};

/*
** Macros to lock/unlock the drawable private
*/
#if defined(DEBUG)
#define __GL_LOCK_DP(glPriv,gc) \
    (*(glPriv)->lockDP)(glPriv,gc); \
    (glPriv)->lockLine[(glPriv)->lockRefCount] = __LINE__; \
    (glPriv)->lockFile[(glPriv)->lockRefCount] = __FILE__; \
    (glPriv)->lockRefCount++
#define __GL_UNLOCK_DP(glPriv) \
    (glPriv)->lockRefCount--; \
    (glPriv)->lockLine[(glPriv)->lockRefCount] = 0; \
    (glPriv)->lockFile[(glPriv)->lockRefCount] = NULL; \
    (*(glPriv)->unlockDP)(glPriv)
#else /* DEBUG */
#define __GL_LOCK_DP(glPriv,gc)		(*(glPriv)->lockDP)(glPriv,gc)
#define	__GL_UNLOCK_DP(glPriv)		(*(glPriv)->unlockDP)(glPriv)
#endif /* DEBUG */


/*
** Procedures which are imported by the GL from the surrounding
** "operating system".  Math functions are not considered part of the
** "operating system".
*/
typedef struct __GLimportsRec {
    /* Memory management */
    void * (*malloc)(__GLcontext *gc, size_t size);
    void *(*calloc)(__GLcontext *gc, size_t numElem, size_t elemSize);
    void *(*realloc)(__GLcontext *gc, void *oldAddr, size_t newSize);
    void (*free)(__GLcontext *gc, void *addr);

    /* Error handling */
    void (*warning)(__GLcontext *gc, char *fmt);
    void (*fatal)(__GLcontext *gc, char *fmt);

    /* other system calls */
    char *(CAPI *getenv)(__GLcontext *gc, const char *var);
    int (CAPI *atoi)(__GLcontext *gc, const char *str);
    int (CAPI *sprintf)(__GLcontext *gc, char *str, const char *fmt, ...);
    void *(CAPI *fopen)(__GLcontext *gc, const char *path, const char *mode);
    int (CAPI *fclose)(__GLcontext *gc, void *stream);
    int (CAPI *fprintf)(__GLcontext *gc, void *stream, const char *fmt, ...);

    /* Drawing surface management */
    __GLdrawablePrivate *(*getDrawablePrivate)(__GLcontext *gc);
    __GLdrawablePrivate *(*getReadablePrivate)(__GLcontext *gc);

    /* Operating system dependent data goes here */
    void *other;
} __GLimports;

/************************************************************************/

/*
** Procedures which are exported by the GL to the surrounding "operating
** system" so that it can manage multiple GL context's.
*/
typedef struct __GLexportsRec {
    /* Context management (return GL_FALSE on failure) */
    GLboolean (*destroyContext)(__GLcontext *gc);
    GLboolean (*loseCurrent)(__GLcontext *gc);
    /* oldglPriv isn't used anymore, kept for backwards compatibility */
    GLboolean (*makeCurrent)(__GLcontext *gc);
    GLboolean (*shareContext)(__GLcontext *gc, __GLcontext *gcShare);
    GLboolean (*copyContext)(__GLcontext *dst, const __GLcontext *src, GLuint mask);
    GLboolean (*forceCurrent)(__GLcontext *gc);

    /* Drawing surface notification callbacks */
    GLboolean (*notifyResize)(__GLcontext *gc);
    void (*notifyDestroy)(__GLcontext *gc);
    void (*notifySwapBuffers)(__GLcontext *gc);

    /* Dispatch table override control for external agents like libGLS */
    struct __GLdispatchStateRec* (*dispatchExec)(__GLcontext *gc);
    void (*beginDispatchOverride)(__GLcontext *gc);
    void (*endDispatchOverride)(__GLcontext *gc);
} __GLexports;

/************************************************************************/

/*
** This must be the first member of a __GLcontext structure.  This is the
** only part of a context that is exposed to the outside world; everything
** else is opaque.
*/
struct __GLinterfaceRec {
    __GLimports imports;
    __GLexports exports;
};

extern __GLcontext *__glCoreCreateContext(__GLimports *, __GLcontextModes *);
extern void __glCoreNopDispatch(void);

#endif /* __gl_core_h_ */
