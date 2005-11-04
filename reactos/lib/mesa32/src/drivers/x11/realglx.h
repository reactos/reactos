
/*
 * Mesa 3-D graphics library
 * Version:  3.5
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


#ifndef REALGLX_H
#define REALGLX_H


extern struct _glxapi_table *
_real_GetGLXDispatchTable(void);


/*
 * Basically just need these to prevent compiler warnings.
 */


extern XVisualInfo *
_real_glXChooseVisual( Display *dpy, int screen, int *list );

extern GLXContext
_real_glXCreateContext( Display *dpy, XVisualInfo *visinfo,
                        GLXContext share_list, Bool direct );

extern GLXPixmap
_real_glXCreateGLXPixmap( Display *dpy, XVisualInfo *visinfo, Pixmap pixmap );

extern GLXPixmap
_real_glXCreateGLXPixmapMESA( Display *dpy, XVisualInfo *visinfo,
                              Pixmap pixmap, Colormap cmap );

extern void
_real_glXDestroyGLXPixmap( Display *dpy, GLXPixmap pixmap );

extern void
_real_glXCopyContext( Display *dpy, GLXContext src, GLXContext dst,
                      unsigned long mask );

extern Bool
_real_glXMakeCurrent( Display *dpy, GLXDrawable drawable, GLXContext ctx );

extern Bool
_real_glXQueryExtension( Display *dpy, int *errorb, int *event );

extern void
_real_glXDestroyContext( Display *dpy, GLXContext ctx );

extern Bool
_real_glXIsDirect( Display *dpy, GLXContext ctx );

extern void
_real_glXSwapBuffers( Display *dpy, GLXDrawable drawable );

extern void
_real_glXUseXFont( Font font, int first, int count, int listbase );

extern Bool
_real_glXQueryVersion( Display *dpy, int *maj, int *min );

extern int
_real_glXGetConfig( Display *dpy, XVisualInfo *visinfo,
                    int attrib, int *value );

extern void
_real_glXWaitGL( void );


extern void
_real_glXWaitX( void );

/* GLX 1.1 and later */
extern const char *
_real_glXQueryExtensionsString( Display *dpy, int screen );

/* GLX 1.1 and later */
extern const char *
_real_glXQueryServerString( Display *dpy, int screen, int name );

/* GLX 1.1 and later */
extern const char *
_real_glXGetClientString( Display *dpy, int name );


/*
 * GLX 1.3 and later
 */

extern GLXFBConfig *
_real_glXChooseFBConfig( Display *dpy, int screen,
                         const int *attribList, int *nitems );

extern int
_real_glXGetFBConfigAttrib( Display *dpy, GLXFBConfig config,
                            int attribute, int *value );

extern GLXFBConfig *
_real_glXGetFBConfigs( Display *dpy, int screen, int *nelements );

extern XVisualInfo *
_real_glXGetVisualFromFBConfig( Display *dpy, GLXFBConfig config );

extern GLXWindow
_real_glXCreateWindow( Display *dpy, GLXFBConfig config, Window win,
                       const int *attribList );

extern void
_real_glXDestroyWindow( Display *dpy, GLXWindow window );

extern GLXPixmap
_real_glXCreatePixmap( Display *dpy, GLXFBConfig config, Pixmap pixmap,
                       const int *attribList );

extern void
_real_glXDestroyPixmap( Display *dpy, GLXPixmap pixmap );

extern GLXPbuffer
_real_glXCreatePbuffer( Display *dpy, GLXFBConfig config,
                        const int *attribList );

extern void
_real_glXDestroyPbuffer( Display *dpy, GLXPbuffer pbuf );

extern void
_real_glXQueryDrawable( Display *dpy, GLXDrawable draw, int attribute,
                        unsigned int *value );

extern GLXContext
_real_glXCreateNewContext( Display *dpy, GLXFBConfig config,
                           int renderType, GLXContext shareList, Bool direct );


extern Bool
_real_glXMakeContextCurrent( Display *dpy, GLXDrawable draw,
                             GLXDrawable read, GLXContext ctx );

extern int
_real_glXQueryContext( Display *dpy, GLXContext ctx, int attribute, int *value );

extern void
_real_glXSelectEvent( Display *dpy, GLXDrawable drawable, unsigned long mask );

extern void
_real_glXGetSelectedEvent( Display *dpy, GLXDrawable drawable,
                           unsigned long *mask );

#ifdef GLX_SGI_swap_control
extern int
_real_glXSwapIntervalSGI(int interval);
#endif


#ifdef GLX_SGI_video_sync
extern int
_real_glXGetVideoSyncSGI(unsigned int *count);

extern int
_real_glXWaitVideoSyncSGI(int divisor, int remainder, unsigned int *count);
#endif


#ifdef GLX_SGI_make_current_read
extern Bool
_real_glXMakeCurrentReadSGI(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx);

extern GLXDrawable
_real_glXGetCurrentReadDrawableSGI(void);
#endif

#if defined(_VL_H) && defined(GLX_SGIX_video_source)
extern GLXVideoSourceSGIX
_real_glXCreateGLXVideoSourceSGIX(Display *dpy, int screen, VLServer server, VLPath path, int nodeClass, VLNode drainNode);

extern void
_real_glXDestroyGLXVideoSourceSGIX(Display *dpy, GLXVideoSourceSGIX src);
#endif

#ifdef GLX_EXT_import_context
extern void
_real_glXFreeContextEXT(Display *dpy, GLXContext context);

extern GLXContextID
_real_glXGetContextIDEXT(const GLXContext context);

extern Display *
_real_glXGetCurrentDisplayEXT(void);

extern GLXContext
_real_glXImportContextEXT(Display *dpy, GLXContextID contextID);

extern int
_real_glXQueryContextInfoEXT(Display *dpy, GLXContext context, int attribute, int *value);
#endif

#ifdef GLX_SGIX_fbconfig
extern int
_real_glXGetFBConfigAttribSGIX(Display *dpy, GLXFBConfigSGIX config, int attribute, int *value);

extern GLXFBConfigSGIX *
_real_glXChooseFBConfigSGIX(Display *dpy, int screen, int *attrib_list, int *nelements);

extern GLXPixmap
_real_glXCreateGLXPixmapWithConfigSGIX(Display *dpy, GLXFBConfigSGIX config, Pixmap pixmap);

extern GLXContext
_real_glXCreateContextWithConfigSGIX(Display *dpy, GLXFBConfigSGIX config, int render_type, GLXContext share_list, Bool direct);

extern XVisualInfo *
_real_glXGetVisualFromFBConfigSGIX(Display *dpy, GLXFBConfigSGIX config);

extern GLXFBConfigSGIX
_real_glXGetFBConfigFromVisualSGIX(Display *dpy, XVisualInfo *vis);
#endif

#ifdef GLX_SGIX_pbuffer
extern GLXPbufferSGIX
_real_glXCreateGLXPbufferSGIX(Display *dpy, GLXFBConfigSGIX config, unsigned int width, unsigned int height, int *attrib_list);

extern void
_real_glXDestroyGLXPbufferSGIX(Display *dpy, GLXPbufferSGIX pbuf);

extern int
_real_glXQueryGLXPbufferSGIX(Display *dpy, GLXPbufferSGIX pbuf, int attribute, unsigned int *value);

extern void
_real_glXSelectEventSGIX(Display *dpy, GLXDrawable drawable, unsigned long mask);

extern void
_real_glXGetSelectedEventSGIX(Display *dpy, GLXDrawable drawable, unsigned long *mask);
#endif

#ifdef GLX_SGI_cushion
extern void
_real_glXCushionSGI(Display *dpy, Window win, float cushion);
#endif

#ifdef GLX_SGIX_video_resize
extern int
_real_glXBindChannelToWindowSGIX(Display *dpy, int screen, int channel , Window window);

extern int
_real_glXChannelRectSGIX(Display *dpy, int screen, int channel, int x, int y, int w, int h);

extern int
_real_glXQueryChannelRectSGIX(Display *dpy, int screen, int channel, int *x, int *y, int *w, int *h);

extern int
_real_glXQueryChannelDeltasSGIX(Display *dpy, int screen, int channel, int *dx, int *dy, int *dw, int *dh);

extern int
_real_glXChannelRectSyncSGIX(Display *dpy, int screen, int channel, GLenum synctype);
#endif

#if defined(_DM_BUFFER_H_) && defined(GLX_SGIX_dmbuffer)
extern Bool
_real_glXAssociateDMPbufferSGIX(Display *dpy, GLXPbufferSGIX pbuffer, DMparams *params, DMbuffer dmbuffer);
#endif

#ifdef GLX_SGIX_swap_group
extern void
_real_glXJoinSwapGroupSGIX(Display *dpy, GLXDrawable drawable, GLXDrawable member);
#endif

#ifdef GLX_SGIX_swap_barrier
extern void
_real_glXBindSwapBarrierSGIX(Display *dpy, GLXDrawable drawable, int barrier);

extern Bool
_real_glXQueryMaxSwapBarriersSGIX(Display *dpy, int screen, int *max);
#endif

#ifdef GLX_SUN_get_transparent_index
extern Status
_real_glXGetTransparentIndexSUN(Display *dpy, Window overlay, Window underlay, long *pTransparent);
#endif

#ifdef GLX_MESA_release_buffers
extern Bool
_real_glXReleaseBuffersMESA( Display *dpy, GLXDrawable d );
#endif

#ifdef GLX_MESA_set_3dfx_mode
extern Bool
_real_glXSet3DfxModeMESA( int mode );
#endif

#ifdef GLX_NV_vertex_array_range
extern void *
_real_glXAllocateMemoryNV(GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority);
extern void
_real_glXFreeMemoryNV(GLvoid *pointer);
#endif

#ifdef GLX_MESA_agp_offset
extern GLuint
_real_glXGetAGPOffsetMESA(const GLvoid *pointer);
#endif

#ifdef GLX_MESA_copy_sub_buffer
extern void
_real_glXCopySubBufferMESA( Display *dpy, GLXDrawable drawable,
                            int x, int y, int width, int height );
#endif

#endif /* REALGLX_H */
