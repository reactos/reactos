/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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


#ifndef GLX_H
#define GLX_H


#ifdef __VMS
#include <GL/vms_x_fix.h>
# ifdef __cplusplus
/* VMS Xlib.h gives problems with C++.
 * this avoids a bunch of trivial warnings */
#pragma message disable nosimpint
#endif
#endif
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef __VMS
# ifdef __cplusplus
#pragma message enable nosimpint
#endif
#endif
#include <GL/gl.h>


#if defined(USE_MGL_NAMESPACE)
#include "glx_mangle.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif


#define GLX_VERSION_1_1		1
#define GLX_VERSION_1_2		1
#define GLX_VERSION_1_3		1
#define GLX_VERSION_1_4		1

#define GLX_EXTENSION_NAME   "GLX"



/*
 * Tokens for glXChooseVisual and glXGetConfig:
 */
#define GLX_USE_GL		1
#define GLX_BUFFER_SIZE		2
#define GLX_LEVEL		3
#define GLX_RGBA		4
#define GLX_DOUBLEBUFFER	5
#define GLX_STEREO		6
#define GLX_AUX_BUFFERS		7
#define GLX_RED_SIZE		8
#define GLX_GREEN_SIZE		9
#define GLX_BLUE_SIZE		10
#define GLX_ALPHA_SIZE		11
#define GLX_DEPTH_SIZE		12
#define GLX_STENCIL_SIZE	13
#define GLX_ACCUM_RED_SIZE	14
#define GLX_ACCUM_GREEN_SIZE	15
#define GLX_ACCUM_BLUE_SIZE	16
#define GLX_ACCUM_ALPHA_SIZE	17


/*
 * Error codes returned by glXGetConfig:
 */
#define GLX_BAD_SCREEN		1
#define GLX_BAD_ATTRIBUTE	2
#define GLX_NO_EXTENSION	3
#define GLX_BAD_VISUAL		4
#define GLX_BAD_CONTEXT		5
#define GLX_BAD_VALUE       	6
#define GLX_BAD_ENUM		7


/*
 * GLX 1.1 and later:
 */
#define GLX_VENDOR		1
#define GLX_VERSION		2
#define GLX_EXTENSIONS 		3


/*
 * GLX 1.3 and later:
 */
#define GLX_CONFIG_CAVEAT		0x20
#define GLX_DONT_CARE			0xFFFFFFFF
#define GLX_X_VISUAL_TYPE		0x22
#define GLX_TRANSPARENT_TYPE		0x23
#define GLX_TRANSPARENT_INDEX_VALUE	0x24
#define GLX_TRANSPARENT_RED_VALUE	0x25
#define GLX_TRANSPARENT_GREEN_VALUE	0x26
#define GLX_TRANSPARENT_BLUE_VALUE	0x27
#define GLX_TRANSPARENT_ALPHA_VALUE	0x28
#define GLX_WINDOW_BIT			0x00000001
#define GLX_PIXMAP_BIT			0x00000002
#define GLX_PBUFFER_BIT			0x00000004
#define GLX_AUX_BUFFERS_BIT		0x00000010
#define GLX_FRONT_LEFT_BUFFER_BIT	0x00000001
#define GLX_FRONT_RIGHT_BUFFER_BIT	0x00000002
#define GLX_BACK_LEFT_BUFFER_BIT	0x00000004
#define GLX_BACK_RIGHT_BUFFER_BIT	0x00000008
#define GLX_DEPTH_BUFFER_BIT		0x00000020
#define GLX_STENCIL_BUFFER_BIT		0x00000040
#define GLX_ACCUM_BUFFER_BIT		0x00000080
#define GLX_NONE			0x8000
#define GLX_SLOW_CONFIG			0x8001
#define GLX_TRUE_COLOR			0x8002
#define GLX_DIRECT_COLOR		0x8003
#define GLX_PSEUDO_COLOR		0x8004
#define GLX_STATIC_COLOR		0x8005
#define GLX_GRAY_SCALE			0x8006
#define GLX_STATIC_GRAY			0x8007
#define GLX_TRANSPARENT_RGB		0x8008
#define GLX_TRANSPARENT_INDEX		0x8009
#define GLX_VISUAL_ID			0x800B
#define GLX_SCREEN			0x800C
#define GLX_NON_CONFORMANT_CONFIG	0x800D
#define GLX_DRAWABLE_TYPE		0x8010
#define GLX_RENDER_TYPE			0x8011
#define GLX_X_RENDERABLE		0x8012
#define GLX_FBCONFIG_ID			0x8013
#define GLX_RGBA_TYPE			0x8014
#define GLX_COLOR_INDEX_TYPE		0x8015
#define GLX_MAX_PBUFFER_WIDTH		0x8016
#define GLX_MAX_PBUFFER_HEIGHT		0x8017
#define GLX_MAX_PBUFFER_PIXELS		0x8018
#define GLX_PRESERVED_CONTENTS		0x801B
#define GLX_LARGEST_PBUFFER		0x801C
#define GLX_WIDTH			0x801D
#define GLX_HEIGHT			0x801E
#define GLX_EVENT_MASK			0x801F
#define GLX_DAMAGED			0x8020
#define GLX_SAVED			0x8021
#define GLX_WINDOW			0x8022
#define GLX_PBUFFER			0x8023
#define GLX_PBUFFER_HEIGHT              0x8040
#define GLX_PBUFFER_WIDTH               0x8041
#define GLX_RGBA_BIT			0x00000001
#define GLX_COLOR_INDEX_BIT		0x00000002
#define GLX_PBUFFER_CLOBBER_MASK	0x08000000


/*
 * GLX 1.4 and later:
 */
#define GLX_SAMPLE_BUFFERS              0x186a0 /*100000*/
#define GLX_SAMPLES                     0x186a1 /*100001*/



typedef struct __GLXcontextRec *GLXContext;
typedef XID GLXPixmap;
typedef XID GLXDrawable;
/* GLX 1.3 and later */
typedef struct __GLXFBConfigRec *GLXFBConfig;
typedef XID GLXFBConfigID;
typedef XID GLXContextID;
typedef XID GLXWindow;
typedef XID GLXPbuffer;



extern XVisualInfo* glXChooseVisual( Display *dpy, int screen,
				     int *attribList );

extern GLXContext glXCreateContext( Display *dpy, XVisualInfo *vis,
				    GLXContext shareList, Bool direct );

extern void glXDestroyContext( Display *dpy, GLXContext ctx );

extern Bool glXMakeCurrent( Display *dpy, GLXDrawable drawable,
			    GLXContext ctx);

extern void glXCopyContext( Display *dpy, GLXContext src, GLXContext dst,
			    unsigned long mask );

extern void glXSwapBuffers( Display *dpy, GLXDrawable drawable );

extern GLXPixmap glXCreateGLXPixmap( Display *dpy, XVisualInfo *visual,
				     Pixmap pixmap );

extern void glXDestroyGLXPixmap( Display *dpy, GLXPixmap pixmap );

extern Bool glXQueryExtension( Display *dpy, int *errorb, int *event );

extern Bool glXQueryVersion( Display *dpy, int *maj, int *min );

extern Bool glXIsDirect( Display *dpy, GLXContext ctx );

extern int glXGetConfig( Display *dpy, XVisualInfo *visual,
			 int attrib, int *value );

extern GLXContext glXGetCurrentContext( void );

extern GLXDrawable glXGetCurrentDrawable( void );

extern void glXWaitGL( void );

extern void glXWaitX( void );

extern void glXUseXFont( Font font, int first, int count, int list );



/* GLX 1.1 and later */
extern const char *glXQueryExtensionsString( Display *dpy, int screen );

extern const char *glXQueryServerString( Display *dpy, int screen, int name );

extern const char *glXGetClientString( Display *dpy, int name );


/* GLX 1.2 and later */
extern Display *glXGetCurrentDisplay( void );


/* GLX 1.3 and later */
extern GLXFBConfig *glXChooseFBConfig( Display *dpy, int screen,
                                       const int *attribList, int *nitems );

extern int glXGetFBConfigAttrib( Display *dpy, GLXFBConfig config,
                                 int attribute, int *value );

extern GLXFBConfig *glXGetFBConfigs( Display *dpy, int screen,
                                     int *nelements );

extern XVisualInfo *glXGetVisualFromFBConfig( Display *dpy,
                                              GLXFBConfig config );

extern GLXWindow glXCreateWindow( Display *dpy, GLXFBConfig config,
                                  Window win, const int *attribList );

extern void glXDestroyWindow( Display *dpy, GLXWindow window );

extern GLXPixmap glXCreatePixmap( Display *dpy, GLXFBConfig config,
                                  Pixmap pixmap, const int *attribList );

extern void glXDestroyPixmap( Display *dpy, GLXPixmap pixmap );

extern GLXPbuffer glXCreatePbuffer( Display *dpy, GLXFBConfig config,
                                    const int *attribList );

extern void glXDestroyPbuffer( Display *dpy, GLXPbuffer pbuf );

extern void glXQueryDrawable( Display *dpy, GLXDrawable draw, int attribute,
                              unsigned int *value );

extern GLXContext glXCreateNewContext( Display *dpy, GLXFBConfig config,
                                       int renderType, GLXContext shareList,
                                       Bool direct );

extern Bool glXMakeContextCurrent( Display *dpy, GLXDrawable draw,
                                   GLXDrawable read, GLXContext ctx );

extern GLXDrawable glXGetCurrentReadDrawable( void );

extern int glXQueryContext( Display *dpy, GLXContext ctx, int attribute,
                            int *value );

extern void glXSelectEvent( Display *dpy, GLXDrawable drawable,
                            unsigned long mask );

extern void glXGetSelectedEvent( Display *dpy, GLXDrawable drawable,
                                 unsigned long *mask );


/* GLX 1.4 and later */
extern void (*glXGetProcAddress(const GLubyte *procname))( void );


#ifndef GLX_GLXEXT_LEGACY

#include <GL/glxext.h>

#else


/*
 * 28. GLX_EXT_visual_info extension
 */
#ifndef GLX_EXT_visual_info
#define GLX_EXT_visual_info		1

#define GLX_X_VISUAL_TYPE_EXT		0x22
#define GLX_TRANSPARENT_TYPE_EXT	0x23
#define GLX_TRANSPARENT_INDEX_VALUE_EXT	0x24
#define GLX_TRANSPARENT_RED_VALUE_EXT	0x25
#define GLX_TRANSPARENT_GREEN_VALUE_EXT	0x26
#define GLX_TRANSPARENT_BLUE_VALUE_EXT	0x27
#define GLX_TRANSPARENT_ALPHA_VALUE_EXT	0x28
#define GLX_TRUE_COLOR_EXT		0x8002
#define GLX_DIRECT_COLOR_EXT		0x8003
#define GLX_PSEUDO_COLOR_EXT		0x8004
#define GLX_STATIC_COLOR_EXT		0x8005
#define GLX_GRAY_SCALE_EXT		0x8006
#define GLX_STATIC_GRAY_EXT		0x8007
#define GLX_NONE_EXT			0x8000
#define GLX_TRANSPARENT_RGB_EXT		0x8008
#define GLX_TRANSPARENT_INDEX_EXT	0x8009

#endif /* 28. GLX_EXT_visual_info extension */



/*
 * 41. GLX_SGI_video_sync
 */
#ifndef GLX_SGI_video_sync
#define GLX_SGI_video_sync 1

extern int glXGetVideoSyncSGI(unsigned int *count);
extern int glXWaitVideoSyncSGI(int divisor, int remainder, unsigned int *count);

#endif /* GLX_SGI_video_sync */



/*
 * 42. GLX_EXT_visual_rating
 */
#ifndef GLX_EXT_visual_rating
#define GLX_EXT_visual_rating		1

#define GLX_VISUAL_CAVEAT_EXT		0x20
/*#define GLX_NONE_EXT			0x8000*/
#define GLX_SLOW_VISUAL_EXT		0x8001
#define GLX_NON_CONFORMANT_VISUAL_EXT	0x800D

#endif /* GLX_EXT_visual_rating	*/



/*
 * 47. GLX_EXT_import_context
 */
#ifndef GLX_EXT_import_context
#define GLX_EXT_import_context 1

#define GLX_SHARE_CONTEXT_EXT		0x800A
#define GLX_VISUAL_ID_EXT		0x800B
#define GLX_SCREEN_EXT			0x800C

extern void glXFreeContextEXT(Display *dpy, GLXContext context);

extern GLXContextID glXGetContextIDEXT(const GLXContext context);

extern Display *glXGetCurrentDisplayEXT(void);

extern GLXContext glXImportContextEXT(Display *dpy, GLXContextID contextID);

extern int glXQueryContextInfoEXT(Display *dpy, GLXContext context,
                                  int attribute,int *value);

#endif /* GLX_EXT_import_context */



/*
 * 215. GLX_MESA_copy_sub_buffer
 */
#ifndef GLX_MESA_copy_sub_buffer
#define GLX_MESA_copy_sub_buffer 1

extern void glXCopySubBufferMESA( Display *dpy, GLXDrawable drawable,
                                  int x, int y, int width, int height );

#endif



/*
 * 216. GLX_MESA_pixmap_colormap
 */
#ifndef GLX_MESA_pixmap_colormap
#define GLX_MESA_pixmap_colormap 1

extern GLXPixmap glXCreateGLXPixmapMESA( Display *dpy, XVisualInfo *visual,
                                         Pixmap pixmap, Colormap cmap );

#endif /* GLX_MESA_pixmap_colormap */



/*
 * 217. GLX_MESA_release_buffers
 */
#ifndef GLX_MESA_release_buffers
#define GLX_MESA_release_buffers 1

extern Bool glXReleaseBuffersMESA( Display *dpy, GLXDrawable d );

#endif /* GLX_MESA_release_buffers */



/*
 * 218. GLX_MESA_set_3dfx_mode
 */
#ifndef GLX_MESA_set_3dfx_mode
#define GLX_MESA_set_3dfx_mode 1

#define GLX_3DFX_WINDOW_MODE_MESA       0x1
#define GLX_3DFX_FULLSCREEN_MODE_MESA   0x2

extern Bool glXSet3DfxModeMESA( int mode );

#endif /* GLX_MESA_set_3dfx_mode */



/*
 * ARB 2. GLX_ARB_get_proc_address
 */
#ifndef GLX_ARB_get_proc_address
#define GLX_ARB_get_proc_address 1

extern void (*glXGetProcAddressARB(const GLubyte *procName))();

#endif /* GLX_ARB_get_proc_address */



#endif /* GLX_GLXEXT_LEGACY */


/**
 ** The following aren't in glxext.h yet.
 **/


/*
 * ???. GLX_NV_vertex_array_range
 */
#ifndef GLX_NV_vertex_array_range
#define GLX_NV_vertex_array_range

extern void *glXAllocateMemoryNV(GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority);
extern void glXFreeMemoryNV(GLvoid *pointer);
typedef void * ( * PFNGLXALLOCATEMEMORYNVPROC) (GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority);
typedef void ( * PFNGLXFREEMEMORYNVPROC) (GLvoid *pointer);

#endif /* GLX_NV_vertex_array_range */



/*
 * ???. GLX_MESA_agp_offset
 */
#ifndef GLX_MESA_agp_offset
#define GLX_MESA_agp_offset 1

extern GLuint glXGetAGPOffsetMESA(const GLvoid *pointer);
typedef GLuint (* PFNGLXGETAGPOFFSETMESAPROC) (const GLvoid *pointer);

#endif /* GLX_MESA_agp_offset */


/*
 * ???. GLX_MESA_allocate_memory
 */
#ifndef GLX_MESA_allocate_memory
#define GLX_MESA_allocate_memory 1

extern void *glXAllocateMemoryMESA(Display *dpy, int scrn, size_t size, float readfreq, float writefreq, float priority);
extern void glXFreeMemoryMESA(Display *dpy, int scrn, void *pointer);
extern GLuint glXGetMemoryOffsetMESA(Display *dpy, int scrn, const void *pointer);
typedef void * ( * PFNGLXALLOCATEMEMORYMESAPROC) (Display *dpy, int scrn, size_t size, float readfreq, float writefreq, float priority);
typedef void ( * PFNGLXFREEMEMORYMESAPROC) (Display *dpy, int scrn, void *pointer);
typedef GLuint (* PFNGLXGETMEMORYOFFSETMESAPROC) (Display *dpy, int scrn, const void *pointer);

#endif /* GLX_MESA_allocate_memory */

/*
 * ARB ?. GLX_ARB_render_texture
 */
#ifndef GLX_ARB_render_texture
#define GLX_ARB_render_texture 1

extern Bool glXBindTexImageARB(Display *dpy, GLXPbuffer pbuffer, int buffer);
extern Bool glXReleaseTexImageARB(Display *dpy, GLXPbuffer pbuffer, int buffer);
extern Bool glXDrawableAttribARB(Display *dpy, GLXDrawable draw, const int *attribList);

#endif /* GLX_ARB_render_texture */


/*
 * Remove this when glxext.h is updated.
 */
#ifndef GLX_NV_float_buffer
#define GLX_NV_float_buffer 1

#define GLX_FLOAT_COMPONENTS_NV         0x20B0

#endif /* GLX_NV_float_buffer */


/*** Should these go here, or in another header? */
/*
** GLX Events
*/
typedef struct {
    int event_type;		/* GLX_DAMAGED or GLX_SAVED */
    int draw_type;		/* GLX_WINDOW or GLX_PBUFFER */
    unsigned long serial;	/* # of last request processed by server */
    Bool send_event;		/* true if this came for SendEvent request */
    Display *display;		/* display the event was read from */
    GLXDrawable drawable;	/* XID of Drawable */
    unsigned int buffer_mask;	/* mask indicating which buffers are affected */
    unsigned int aux_buffer;	/* which aux buffer was affected */
    int x, y;
    int width, height;
    int count;			/* if nonzero, at least this many more */
} GLXPbufferClobberEvent;

typedef union __GLXEvent {
    GLXPbufferClobberEvent glxpbufferclobber;
    long pad[24];
} GLXEvent;

#ifdef __cplusplus
}
#endif

#endif
