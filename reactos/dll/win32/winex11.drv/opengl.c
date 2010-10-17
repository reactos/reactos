/*
 * X11DRV OpenGL functions
 *
 * Copyright 2000 Lionel Ulmer
 * Copyright 2005 Alex Woods
 * Copyright 2005 Raphael Junqueira
 * Copyright 2006-2009 Roderick Colenbrander
 * Copyright 2006 Tomas Carnecky
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#include "x11drv.h"
#include "winternl.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wgl);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#ifdef SONAME_LIBGL

#undef APIENTRY
#undef CALLBACK
#undef WINAPI

#ifdef HAVE_GL_GL_H
# include <GL/gl.h>
#endif
#ifdef HAVE_GL_GLX_H
# include <GL/glx.h>
#endif

#include "wine/wgl.h"

#undef APIENTRY
#undef CALLBACK
#undef WINAPI

/* Redefines the constants */
#define CALLBACK    __stdcall
#define WINAPI      __stdcall
#define APIENTRY    WINAPI


WINE_DECLARE_DEBUG_CHANNEL(fps);

typedef struct wine_glextension {
    const char *extName;
    struct {
        const char *funcName;
        void *funcAddress;
    } extEntryPoints[9];
} WineGLExtension;

struct WineGLInfo {
    const char *glVersion;
    char *glExtensions;

    int glxVersion[2];

    const char *glxServerVersion;
    const char *glxServerVendor;
    const char *glxServerExtensions;

    const char *glxClientVersion;
    const char *glxClientVendor;
    const char *glxClientExtensions;

    const char *glxExtensions;

    BOOL glxDirect;
    char wglExtensions[4096];
};

typedef struct wine_glpixelformat {
    int         iPixelFormat;
    GLXFBConfig fbconfig;
    int         fmt_id;
    int         render_type;
    BOOL        offscreenOnly;
    DWORD       dwFlags; /* We store some PFD_* flags in here for emulated bitmap formats */
} WineGLPixelFormat;

typedef struct wine_glcontext {
    HDC hdc;
    BOOL do_escape;
    BOOL has_been_current;
    BOOL sharing;
    DWORD tid;
    BOOL gl3_context;
    XVisualInfo *vis;
    WineGLPixelFormat *fmt;
    int numAttribs; /* This is needed for delaying wglCreateContextAttribsARB */
    int attribList[16]; /* This is needed for delaying wglCreateContextAttribsARB */
    GLXContext ctx;
    HDC read_hdc;
    Drawable drawables[2];
    BOOL refresh_drawables;
    struct wine_glcontext *next;
    struct wine_glcontext *prev;
} Wine_GLContext;

typedef struct wine_glpbuffer {
    Drawable   drawable;
    Display*   display;
    WineGLPixelFormat* fmt;
    int        width;
    int        height;
    int*       attribList;
    HDC        hdc;

    int        use_render_texture; /* This is also the internal texture format */
    int        texture_bind_target;
    int        texture_bpp;
    GLint      texture_format;
    GLuint     texture_target;
    GLenum     texture_type;
    GLuint     texture;
    int        texture_level;
} Wine_GLPBuffer;

static Wine_GLContext *context_list;
static struct WineGLInfo WineGLInfo = { 0 };
static int use_render_texture_emulation = 1;
static int use_render_texture_ati = 0;
static int swap_interval = 1;

#define MAX_EXTENSIONS 16
static const WineGLExtension *WineGLExtensionList[MAX_EXTENSIONS];
static int WineGLExtensionListSize;

static void X11DRV_WineGL_LoadExtensions(void);
static WineGLPixelFormat* ConvertPixelFormatWGLtoGLX(Display *display, int iPixelFormat, BOOL AllowOffscreen, int *fmt_count);
static BOOL glxRequireVersion(int requiredVersion);
static BOOL glxRequireExtension(const char *requiredExtension);

static void dump_PIXELFORMATDESCRIPTOR(const PIXELFORMATDESCRIPTOR *ppfd) {
  TRACE("  - size / version : %d / %d\n", ppfd->nSize, ppfd->nVersion);
  TRACE("  - dwFlags : ");
#define TEST_AND_DUMP(t,tv) if ((t) & (tv)) TRACE(#tv " ")
  TEST_AND_DUMP(ppfd->dwFlags, PFD_DEPTH_DONTCARE);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_DOUBLEBUFFER);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_DOUBLEBUFFER_DONTCARE);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_DRAW_TO_WINDOW);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_DRAW_TO_BITMAP);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_GENERIC_ACCELERATED);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_GENERIC_FORMAT);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_NEED_PALETTE);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_NEED_SYSTEM_PALETTE);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_STEREO);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_STEREO_DONTCARE);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_SUPPORT_GDI);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_SUPPORT_OPENGL);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_SWAP_COPY);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_SWAP_EXCHANGE);
  TEST_AND_DUMP(ppfd->dwFlags, PFD_SWAP_LAYER_BUFFERS);
  /* PFD_SUPPORT_COMPOSITION is new in Vista, it is similar to composition
   * under X e.g. COMPOSITE + GLX_EXT_TEXTURE_FROM_PIXMAP. */
  TEST_AND_DUMP(ppfd->dwFlags, PFD_SUPPORT_COMPOSITION);
#undef TEST_AND_DUMP
  TRACE("\n");

  TRACE("  - iPixelType : ");
  switch (ppfd->iPixelType) {
  case PFD_TYPE_RGBA: TRACE("PFD_TYPE_RGBA"); break;
  case PFD_TYPE_COLORINDEX: TRACE("PFD_TYPE_COLORINDEX"); break;
  }
  TRACE("\n");

  TRACE("  - Color   : %d\n", ppfd->cColorBits);
  TRACE("  - Red     : %d\n", ppfd->cRedBits);
  TRACE("  - Green   : %d\n", ppfd->cGreenBits);
  TRACE("  - Blue    : %d\n", ppfd->cBlueBits);
  TRACE("  - Alpha   : %d\n", ppfd->cAlphaBits);
  TRACE("  - Accum   : %d\n", ppfd->cAccumBits);
  TRACE("  - Depth   : %d\n", ppfd->cDepthBits);
  TRACE("  - Stencil : %d\n", ppfd->cStencilBits);
  TRACE("  - Aux     : %d\n", ppfd->cAuxBuffers);

  TRACE("  - iLayerType : ");
  switch (ppfd->iLayerType) {
  case PFD_MAIN_PLANE: TRACE("PFD_MAIN_PLANE"); break;
  case PFD_OVERLAY_PLANE: TRACE("PFD_OVERLAY_PLANE"); break;
  case (BYTE)PFD_UNDERLAY_PLANE: TRACE("PFD_UNDERLAY_PLANE"); break;
  }
  TRACE("\n");
}

#define PUSH1(attribs,att)        do { attribs[nAttribs++] = (att); } while (0)
#define PUSH2(attribs,att,value)  do { attribs[nAttribs++] = (att); attribs[nAttribs++] = (value); } while(0)

#define MAKE_FUNCPTR(f) static typeof(f) * p##f;
/* GLX 1.0 */
MAKE_FUNCPTR(glXChooseVisual)
MAKE_FUNCPTR(glXCopyContext)
MAKE_FUNCPTR(glXCreateContext)
MAKE_FUNCPTR(glXCreateGLXPixmap)
MAKE_FUNCPTR(glXGetCurrentContext)
MAKE_FUNCPTR(glXGetCurrentDrawable)
MAKE_FUNCPTR(glXDestroyContext)
MAKE_FUNCPTR(glXDestroyGLXPixmap)
MAKE_FUNCPTR(glXGetConfig)
MAKE_FUNCPTR(glXIsDirect)
MAKE_FUNCPTR(glXMakeCurrent)
MAKE_FUNCPTR(glXSwapBuffers)
MAKE_FUNCPTR(glXQueryExtension)
MAKE_FUNCPTR(glXQueryVersion)
MAKE_FUNCPTR(glXUseXFont)

/* GLX 1.1 */
MAKE_FUNCPTR(glXGetClientString)
MAKE_FUNCPTR(glXQueryExtensionsString)
MAKE_FUNCPTR(glXQueryServerString)

/* GLX 1.3 */
MAKE_FUNCPTR(glXGetFBConfigs)
MAKE_FUNCPTR(glXChooseFBConfig)
MAKE_FUNCPTR(glXCreatePbuffer)
MAKE_FUNCPTR(glXCreateNewContext)
MAKE_FUNCPTR(glXDestroyPbuffer)
MAKE_FUNCPTR(glXGetFBConfigAttrib)
MAKE_FUNCPTR(glXGetVisualFromFBConfig)
MAKE_FUNCPTR(glXMakeContextCurrent)
MAKE_FUNCPTR(glXQueryDrawable)
MAKE_FUNCPTR(glXGetCurrentReadDrawable)

/* GLX Extensions */
static GLXContext (*pglXCreateContextAttribsARB)(Display *dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list);
static void* (*pglXGetProcAddressARB)(const GLubyte *);
static int   (*pglXSwapIntervalSGI)(int);

/* ATI GLX Extensions */
static BOOL  (*pglXBindTexImageATI)(Display *dpy, GLXPbuffer pbuffer, int buffer);
static BOOL  (*pglXReleaseTexImageATI)(Display *dpy, GLXPbuffer pbuffer, int buffer);
static BOOL  (*pglXDrawableAttribATI)(Display *dpy, GLXDrawable draw, const int *attribList);

/* NV GLX Extension */
static void* (*pglXAllocateMemoryNV)(GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority);
static void  (*pglXFreeMemoryNV)(GLvoid *pointer);

/* MESA GLX Extensions */
static void (*pglXCopySubBufferMESA)(Display *dpy, GLXDrawable drawable, int x, int y, int width, int height);

/* Standard OpenGL */
MAKE_FUNCPTR(glBindTexture)
MAKE_FUNCPTR(glBitmap)
MAKE_FUNCPTR(glCopyTexSubImage1D)
MAKE_FUNCPTR(glCopyTexImage2D)
MAKE_FUNCPTR(glCopyTexSubImage2D)
MAKE_FUNCPTR(glDrawBuffer)
MAKE_FUNCPTR(glEndList)
MAKE_FUNCPTR(glGetError)
MAKE_FUNCPTR(glGetIntegerv)
MAKE_FUNCPTR(glGetString)
MAKE_FUNCPTR(glNewList)
MAKE_FUNCPTR(glPixelStorei)
MAKE_FUNCPTR(glReadPixels)
MAKE_FUNCPTR(glTexImage2D)
MAKE_FUNCPTR(glFinish)
MAKE_FUNCPTR(glFlush)
#undef MAKE_FUNCPTR

static int GLXErrorHandler(Display *dpy, XErrorEvent *event, void *arg)
{
    /* In the future we might want to find the exact X or GLX error to report back to the app */
    return 1;
}

static BOOL infoInitialized = FALSE;
static BOOL X11DRV_WineGL_InitOpenglInfo(void)
{
    int screen = DefaultScreen(gdi_display);
    Window win = 0, root = 0;
    const char* str;
    XVisualInfo *vis;
    GLXContext ctx = NULL;
    XSetWindowAttributes attr;
    BOOL ret = FALSE;
    int attribList[] = {GLX_RGBA, GLX_DOUBLEBUFFER, None};

    if (infoInitialized)
        return TRUE;
    infoInitialized = TRUE;

    attr.override_redirect = True;
    attr.colormap = None;

    wine_tsx11_lock();

    vis = pglXChooseVisual(gdi_display, screen, attribList);
    if (vis) {
#ifdef __i386__
        WORD old_fs = wine_get_fs();
        /* Create a GLX Context. Without one we can't query GL information */
        ctx = pglXCreateContext(gdi_display, vis, None, GL_TRUE);
        if (wine_get_fs() != old_fs)
        {
            wine_set_fs( old_fs );
            ERR( "%%fs register corrupted, probably broken ATI driver, disabling OpenGL.\n" );
            ERR( "You need to set the \"UseFastTls\" option to \"2\" in your X config file.\n" );
            goto done;
        }
#else
        ctx = pglXCreateContext(gdi_display, vis, None, GL_TRUE);
#endif
    }
    if (!ctx) goto done;

    root = RootWindow( gdi_display, vis->screen );
    if (vis->visual != DefaultVisual( gdi_display, vis->screen ))
        attr.colormap = XCreateColormap( gdi_display, root, vis->visual, AllocNone );
    if ((win = XCreateWindow( gdi_display, root, -1, -1, 1, 1, 0, vis->depth, InputOutput,
                              vis->visual, CWOverrideRedirect | CWColormap, &attr )))
        XMapWindow( gdi_display, win );
    else
        win = root;

    pglXMakeCurrent(gdi_display, win, ctx);

    WineGLInfo.glVersion = (const char *) pglGetString(GL_VERSION);
    str = (const char *) pglGetString(GL_EXTENSIONS);
    WineGLInfo.glExtensions = HeapAlloc(GetProcessHeap(), 0, strlen(str)+1);
    strcpy(WineGLInfo.glExtensions, str);

    /* Get the common GLX version supported by GLX client and server ( major/minor) */
    pglXQueryVersion(gdi_display, &WineGLInfo.glxVersion[0], &WineGLInfo.glxVersion[1]);

    WineGLInfo.glxServerVersion = pglXQueryServerString(gdi_display, screen, GLX_VERSION);
    WineGLInfo.glxServerVendor = pglXQueryServerString(gdi_display, screen, GLX_VENDOR);
    WineGLInfo.glxServerExtensions = pglXQueryServerString(gdi_display, screen, GLX_EXTENSIONS);

    WineGLInfo.glxClientVersion = pglXGetClientString(gdi_display, GLX_VERSION);
    WineGLInfo.glxClientVendor = pglXGetClientString(gdi_display, GLX_VENDOR);
    WineGLInfo.glxClientExtensions = pglXGetClientString(gdi_display, GLX_EXTENSIONS);

    WineGLInfo.glxExtensions = pglXQueryExtensionsString(gdi_display, screen);
    WineGLInfo.glxDirect = pglXIsDirect(gdi_display, ctx);

    TRACE("GL version             : %s.\n", WineGLInfo.glVersion);
    TRACE("GL renderer            : %s.\n", pglGetString(GL_RENDERER));
    TRACE("GLX version            : %d.%d.\n", WineGLInfo.glxVersion[0], WineGLInfo.glxVersion[1]);
    TRACE("Server GLX version     : %s.\n", WineGLInfo.glxServerVersion);
    TRACE("Server GLX vendor:     : %s.\n", WineGLInfo.glxServerVendor);
    TRACE("Client GLX version     : %s.\n", WineGLInfo.glxClientVersion);
    TRACE("Client GLX vendor:     : %s.\n", WineGLInfo.glxClientVendor);
    TRACE("Direct rendering enabled: %s\n", WineGLInfo.glxDirect ? "True" : "False");

    if(!WineGLInfo.glxDirect)
    {
        int fd = ConnectionNumber(gdi_display);
        struct sockaddr_un uaddr;
        unsigned int uaddrlen = sizeof(struct sockaddr_un);

        /* In general indirect rendering on a local X11 server indicates a driver problem.
         * Detect a local X11 server by checking whether the X11 socket is a Unix socket.
         */
        if(!getsockname(fd, (struct sockaddr *)&uaddr, &uaddrlen) && uaddr.sun_family == AF_UNIX)
            ERR_(winediag)("Direct rendering is disabled, most likely your OpenGL drivers haven't been installed correctly\n");
    }
    else
    {
        /* In general you would expect that if direct rendering is returned, that you receive hardware
         * accelerated OpenGL rendering. The definition of direct rendering is that rendering is performed
         * client side without sending all GL commands to X using the GLX protocol. When Mesa falls back to
         * software rendering, it shows direct rendering.
         *
         * Depending on the cause of software rendering a different rendering string is shown. In case Mesa fails
         * to load a DRI module 'Software Rasterizer' is returned. When Mesa is compiled as a OpenGL reference driver
         * it shows 'Mesa X11'.
         */
        const char *gl_renderer = (const char *)pglGetString(GL_RENDERER);
        if(!strcmp(gl_renderer, "Software Rasterizer") || !strcmp(gl_renderer, "Mesa X11"))
            ERR_(winediag)("The Mesa OpenGL driver is using software rendering, most likely your OpenGL drivers haven't been installed correctly\n");
    }
    ret = TRUE;

done:
    if(vis) XFree(vis);
    if(ctx) {
        pglXMakeCurrent(gdi_display, None, NULL);    
        pglXDestroyContext(gdi_display, ctx);
    }
    if (win != root) XDestroyWindow( gdi_display, win );
    if (attr.colormap) XFreeColormap( gdi_display, attr.colormap );
    wine_tsx11_unlock();
    if (!ret) ERR(" couldn't initialize OpenGL, expect problems\n");
    return ret;
}

void X11DRV_OpenGL_Cleanup(void)
{
    HeapFree(GetProcessHeap(), 0, WineGLInfo.glExtensions);
    infoInitialized = FALSE;
}

static BOOL has_opengl(void)
{
    static int init_done;
    static void *opengl_handle;

    char buffer[200];
    int error_base, event_base;

    if (init_done) return (opengl_handle != NULL);
    init_done = 1;

    /* No need to load any other libraries as according to the ABI, libGL should be self-sufficient
       and include all dependencies */
    opengl_handle = wine_dlopen(SONAME_LIBGL, RTLD_NOW|RTLD_GLOBAL, buffer, sizeof(buffer));
    if (opengl_handle == NULL)
    {
        ERR( "Failed to load libGL: %s\n", buffer );
        ERR( "OpenGL support is disabled.\n");
        return FALSE;
    }

    pglXGetProcAddressARB = wine_dlsym(opengl_handle, "glXGetProcAddressARB", NULL, 0);
    if (pglXGetProcAddressARB == NULL) {
        ERR("Could not find glXGetProcAddressARB in libGL, disabling OpenGL.\n");
        goto failed;
    }

#define LOAD_FUNCPTR(f) do if((p##f = (void*)pglXGetProcAddressARB((const unsigned char*)#f)) == NULL) \
    { \
        ERR( "%s not found in libGL, disabling OpenGL.\n", #f ); \
        goto failed; \
    } while(0)

    /* GLX 1.0 */
    LOAD_FUNCPTR(glXChooseVisual);
    LOAD_FUNCPTR(glXCopyContext);
    LOAD_FUNCPTR(glXCreateContext);
    LOAD_FUNCPTR(glXCreateGLXPixmap);
    LOAD_FUNCPTR(glXGetCurrentContext);
    LOAD_FUNCPTR(glXGetCurrentDrawable);
    LOAD_FUNCPTR(glXDestroyContext);
    LOAD_FUNCPTR(glXDestroyGLXPixmap);
    LOAD_FUNCPTR(glXGetConfig);
    LOAD_FUNCPTR(glXIsDirect);
    LOAD_FUNCPTR(glXMakeCurrent);
    LOAD_FUNCPTR(glXSwapBuffers);
    LOAD_FUNCPTR(glXQueryExtension);
    LOAD_FUNCPTR(glXQueryVersion);
    LOAD_FUNCPTR(glXUseXFont);

    /* GLX 1.1 */
    LOAD_FUNCPTR(glXGetClientString);
    LOAD_FUNCPTR(glXQueryExtensionsString);
    LOAD_FUNCPTR(glXQueryServerString);

    /* GLX 1.3 */
    LOAD_FUNCPTR(glXCreatePbuffer);
    LOAD_FUNCPTR(glXCreateNewContext);
    LOAD_FUNCPTR(glXDestroyPbuffer);
    LOAD_FUNCPTR(glXMakeContextCurrent);
    LOAD_FUNCPTR(glXGetCurrentReadDrawable);
    LOAD_FUNCPTR(glXGetFBConfigs);

    /* Standard OpenGL calls */
    LOAD_FUNCPTR(glBindTexture);
    LOAD_FUNCPTR(glBitmap);
    LOAD_FUNCPTR(glCopyTexSubImage1D);
    LOAD_FUNCPTR(glCopyTexImage2D);
    LOAD_FUNCPTR(glCopyTexSubImage2D);
    LOAD_FUNCPTR(glDrawBuffer);
    LOAD_FUNCPTR(glEndList);
    LOAD_FUNCPTR(glGetError);
    LOAD_FUNCPTR(glGetIntegerv);
    LOAD_FUNCPTR(glGetString);
    LOAD_FUNCPTR(glNewList);
    LOAD_FUNCPTR(glPixelStorei);
    LOAD_FUNCPTR(glReadPixels);
    LOAD_FUNCPTR(glTexImage2D);
    LOAD_FUNCPTR(glFinish);
    LOAD_FUNCPTR(glFlush);
#undef LOAD_FUNCPTR

/* It doesn't matter if these fail. They'll only be used if the driver reports
   the associated extension is available (and if a driver reports the extension
   is available but fails to provide the functions, it's quite broken) */
#define LOAD_FUNCPTR(f) p##f = (void*)pglXGetProcAddressARB((const unsigned char*)#f)
    /* ARB GLX Extension */
    LOAD_FUNCPTR(glXCreateContextAttribsARB);
    /* NV GLX Extension */
    LOAD_FUNCPTR(glXAllocateMemoryNV);
    LOAD_FUNCPTR(glXFreeMemoryNV);
#undef LOAD_FUNCPTR

    if(!X11DRV_WineGL_InitOpenglInfo()) goto failed;

    wine_tsx11_lock();
    if (pglXQueryExtension(gdi_display, &error_base, &event_base)) {
        TRACE("GLX is up and running error_base = %d\n", error_base);
    } else {
        wine_tsx11_unlock();
        ERR( "GLX extension is missing, disabling OpenGL.\n" );
        goto failed;
    }

    /* In case of GLX you have direct and indirect rendering. Most of the time direct rendering is used
     * as in general only that is hardware accelerated. In some cases like in case of remote X indirect
     * rendering is used.
     *
     * The main problem for our OpenGL code is that we need certain GLX calls but their presence
     * depends on the reported GLX client / server version and on the client / server extension list.
     * Those don't have to be the same.
     *
     * In general the server GLX information lists the capabilities in case of indirect rendering.
     * When direct rendering is used, the OpenGL client library is responsible for which GLX calls are
     * available and in that case the client GLX informat can be used.
     * OpenGL programs should use the 'intersection' of both sets of information which is advertised
     * in the GLX version/extension list. When a program does this it works for certain for both
     * direct and indirect rendering.
     *
     * The problem we are having in this area is that ATI's Linux drivers are broken. For some reason
     * they haven't added some very important GLX extensions like GLX_SGIX_fbconfig to their client
     * extension list which causes this extension not to be listed. (Wine requires this extension).
     * ATI advertises a GLX client version of 1.3 which implies that this fbconfig extension among
     * pbuffers is around.
     *
     * In order to provide users of Ati's proprietary drivers with OpenGL support, we need to detect
     * the ATI drivers and from then on use GLX client information for them.
     */

    if(glxRequireVersion(3)) {
        pglXChooseFBConfig = pglXGetProcAddressARB((const GLubyte *) "glXChooseFBConfig");
        pglXGetFBConfigAttrib = pglXGetProcAddressARB((const GLubyte *) "glXGetFBConfigAttrib");
        pglXGetVisualFromFBConfig = pglXGetProcAddressARB((const GLubyte *) "glXGetVisualFromFBConfig");
        pglXQueryDrawable = pglXGetProcAddressARB((const GLubyte *) "glXQueryDrawable");
    } else if(glxRequireExtension("GLX_SGIX_fbconfig")) {
        pglXChooseFBConfig = pglXGetProcAddressARB((const GLubyte *) "glXChooseFBConfigSGIX");
        pglXGetFBConfigAttrib = pglXGetProcAddressARB((const GLubyte *) "glXGetFBConfigAttribSGIX");
        pglXGetVisualFromFBConfig = pglXGetProcAddressARB((const GLubyte *) "glXGetVisualFromFBConfigSGIX");

        /* The mesa libGL client library seems to forward glXQueryDrawable to the Xserver, so only
         * enable this function when the Xserver understand GLX 1.3 or newer
         */
        pglXQueryDrawable = NULL;
     } else if(strcmp("ATI", WineGLInfo.glxClientVendor) == 0) {
        TRACE("Overriding ATI GLX capabilities!\n");
        pglXChooseFBConfig = pglXGetProcAddressARB((const GLubyte *) "glXChooseFBConfig");
        pglXGetFBConfigAttrib = pglXGetProcAddressARB((const GLubyte *) "glXGetFBConfigAttrib");
        pglXGetVisualFromFBConfig = pglXGetProcAddressARB((const GLubyte *) "glXGetVisualFromFBConfig");
        pglXQueryDrawable = pglXGetProcAddressARB((const GLubyte *) "glXQueryDrawable");

        /* Use client GLX information in case of the ATI drivers. We override the
         * capabilities over here and not somewhere else as ATI might better their
         * life in the future. In case they release proper drivers this block of
         * code won't be called. */
        WineGLInfo.glxExtensions = WineGLInfo.glxClientExtensions;
    } else {
         ERR(" glx_version is %s and GLX_SGIX_fbconfig extension is unsupported. Expect problems.\n", WineGLInfo.glxServerVersion);
    }

    if(glxRequireExtension("GLX_ATI_render_texture")) {
        use_render_texture_ati = 1;
        pglXBindTexImageATI = pglXGetProcAddressARB((const GLubyte *) "glXBindTexImageATI");
        pglXReleaseTexImageATI = pglXGetProcAddressARB((const GLubyte *) "glXReleaseTexImageATI");
        pglXDrawableAttribATI = pglXGetProcAddressARB((const GLubyte *) "glXDrawableAttribATI");
    }

    if(glxRequireExtension("GLX_MESA_copy_sub_buffer")) {
        pglXCopySubBufferMESA = pglXGetProcAddressARB((const GLubyte *) "glXCopySubBufferMESA");
    }

    X11DRV_WineGL_LoadExtensions();

    wine_tsx11_unlock();
    return TRUE;

failed:
    wine_dlclose(opengl_handle, NULL, 0);
    opengl_handle = NULL;
    return FALSE;
}

static inline Wine_GLContext *alloc_context(void)
{
    Wine_GLContext *ret;

    if ((ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Wine_GLContext))))
    {
        ret->next = context_list;
        if (context_list) context_list->prev = ret;
        context_list = ret;
    }
    return ret;
}

static inline void free_context(Wine_GLContext *context)
{
    if (context->next != NULL) context->next->prev = context->prev;
    if (context->prev != NULL) context->prev->next = context->next;
    else context_list = context->next;

    if (context->vis) XFree(context->vis);
    HeapFree(GetProcessHeap(), 0, context);
}

static inline BOOL is_valid_context( Wine_GLContext *ctx )
{
    Wine_GLContext *ptr;
    for (ptr = context_list; ptr; ptr = ptr->next) if (ptr == ctx) break;
    return (ptr != NULL);
}

static int describeContext(Wine_GLContext* ctx) {
    int tmp;
    int ctx_vis_id;
    TRACE(" Context %p have (vis:%p):\n", ctx, ctx->vis);
    pglXGetFBConfigAttrib(gdi_display, ctx->fmt->fbconfig, GLX_FBCONFIG_ID, &tmp);
    TRACE(" - FBCONFIG_ID 0x%x\n", tmp);
    pglXGetFBConfigAttrib(gdi_display, ctx->fmt->fbconfig, GLX_VISUAL_ID, &tmp);
    TRACE(" - VISUAL_ID 0x%x\n", tmp);
    ctx_vis_id = tmp;
    return ctx_vis_id;
}

static BOOL describeDrawable(X11DRV_PDEVICE *physDev) {
    int tmp;
    WineGLPixelFormat *fmt;
    int fmt_count = 0;

    fmt = ConvertPixelFormatWGLtoGLX(gdi_display, physDev->current_pf, TRUE /* Offscreen */, &fmt_count);
    if(!fmt) return FALSE;

    TRACE(" HDC %p has:\n", physDev->hdc);
    TRACE(" - iPixelFormat %d\n", fmt->iPixelFormat);
    TRACE(" - Drawable %p\n", (void*) get_glxdrawable(physDev));
    TRACE(" - FBCONFIG_ID 0x%x\n", fmt->fmt_id);

    pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_VISUAL_ID, &tmp);
    TRACE(" - VISUAL_ID 0x%x\n", tmp);

    return TRUE;
}

static int ConvertAttribWGLtoGLX(const int* iWGLAttr, int* oGLXAttr, Wine_GLPBuffer* pbuf) {
  int nAttribs = 0;
  unsigned cur = 0; 
  int pop;
  int drawattrib = 0;
  int nvfloatattrib = GLX_DONT_CARE;
  int pixelattrib = ~0;

  /* The list of WGL attributes is allowed to be NULL. We don't return here for NULL
   * because we need to do fixups for GLX_DRAWABLE_TYPE/GLX_RENDER_TYPE/GLX_FLOAT_COMPONENTS_NV. */
  while (iWGLAttr && 0 != iWGLAttr[cur]) {
    TRACE("pAttr[%d] = %x\n", cur, iWGLAttr[cur]);

    switch (iWGLAttr[cur]) {
    case WGL_AUX_BUFFERS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_AUX_BUFFERS, pop);
      TRACE("pAttr[%d] = GLX_AUX_BUFFERS: %d\n", cur, pop);
      break;
    case WGL_COLOR_BITS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_BUFFER_SIZE, pop);
      TRACE("pAttr[%d] = GLX_BUFFER_SIZE: %d\n", cur, pop);
      break;
    case WGL_BLUE_BITS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_BLUE_SIZE, pop);
      TRACE("pAttr[%d] = GLX_BLUE_SIZE: %d\n", cur, pop);
      break;
    case WGL_RED_BITS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_RED_SIZE, pop);
      TRACE("pAttr[%d] = GLX_RED_SIZE: %d\n", cur, pop);
      break;
    case WGL_GREEN_BITS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_GREEN_SIZE, pop);
      TRACE("pAttr[%d] = GLX_GREEN_SIZE: %d\n", cur, pop);
      break;
    case WGL_ALPHA_BITS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_ALPHA_SIZE, pop);
      TRACE("pAttr[%d] = GLX_ALPHA_SIZE: %d\n", cur, pop);
      break;
    case WGL_DEPTH_BITS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_DEPTH_SIZE, pop);
      TRACE("pAttr[%d] = GLX_DEPTH_SIZE: %d\n", cur, pop);
      break;
    case WGL_STENCIL_BITS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_STENCIL_SIZE, pop);
      TRACE("pAttr[%d] = GLX_STENCIL_SIZE: %d\n", cur, pop);
      break;
    case WGL_DOUBLE_BUFFER_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_DOUBLEBUFFER, pop);
      TRACE("pAttr[%d] = GLX_DOUBLEBUFFER: %d\n", cur, pop);
      break;
    case WGL_STEREO_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_STEREO, pop);
      TRACE("pAttr[%d] = GLX_STEREO: %d\n", cur, pop);
      break;

    case WGL_PIXEL_TYPE_ARB:
      pop = iWGLAttr[++cur];
      TRACE("pAttr[%d] = WGL_PIXEL_TYPE_ARB: %d\n", cur, pop);
      switch (pop) {
      case WGL_TYPE_COLORINDEX_ARB: pixelattrib = GLX_COLOR_INDEX_BIT; break ;
      case WGL_TYPE_RGBA_ARB: pixelattrib = GLX_RGBA_BIT; break ;
      /* This is the same as WGL_TYPE_RGBA_FLOAT_ATI but the GLX constants differ, only the ARB GLX one is widely supported so use that */
      case WGL_TYPE_RGBA_FLOAT_ATI: pixelattrib = GLX_RGBA_FLOAT_BIT; break ;
      case WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT: pixelattrib = GLX_RGBA_UNSIGNED_FLOAT_BIT_EXT; break ;
      default:
        ERR("unexpected PixelType(%x)\n", pop);	
        pop = 0;
      }
      break;

    case WGL_SUPPORT_GDI_ARB:
      /* This flag is set in a WineGLPixelFormat */
      pop = iWGLAttr[++cur];
      TRACE("pAttr[%d] = WGL_SUPPORT_GDI_ARB: %d\n", cur, pop);
      break;

    case WGL_DRAW_TO_BITMAP_ARB:
      /* This flag is set in a WineGLPixelFormat */
      pop = iWGLAttr[++cur];
      TRACE("pAttr[%d] = WGL_DRAW_TO_BITMAP_ARB: %d\n", cur, pop);
      break;

    case WGL_DRAW_TO_WINDOW_ARB:
      pop = iWGLAttr[++cur];
      TRACE("pAttr[%d] = WGL_DRAW_TO_WINDOW_ARB: %d\n", cur, pop);
      /* GLX_DRAWABLE_TYPE flags need to be OR'd together. See below. */
      if (pop) {
        drawattrib |= GLX_WINDOW_BIT;
      }
      break;

    case WGL_DRAW_TO_PBUFFER_ARB:
      pop = iWGLAttr[++cur];
      TRACE("pAttr[%d] = WGL_DRAW_TO_PBUFFER_ARB: %d\n", cur, pop);
      /* GLX_DRAWABLE_TYPE flags need to be OR'd together. See below. */
      if (pop) {
        drawattrib |= GLX_PBUFFER_BIT;
      }
      break;

    case WGL_ACCELERATION_ARB:
      /* This flag is set in a WineGLPixelFormat */
      pop = iWGLAttr[++cur];
      TRACE("pAttr[%d] = WGL_ACCELERATION_ARB: %d\n", cur, pop);
      break;

    case WGL_SUPPORT_OPENGL_ARB:
      pop = iWGLAttr[++cur];
      /** nothing to do, if we are here, supposing support Accelerated OpenGL */
      TRACE("pAttr[%d] = WGL_SUPPORT_OPENGL_ARB: %d\n", cur, pop);
      break;

    case WGL_SWAP_METHOD_ARB:
      pop = iWGLAttr[++cur];
      /* For now we ignore this and just return SWAP_EXCHANGE */
      TRACE("pAttr[%d] = WGL_SWAP_METHOD_ARB: %#x\n", cur, pop);
      break;

    case WGL_PBUFFER_LARGEST_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_LARGEST_PBUFFER, pop);
      TRACE("pAttr[%d] = GLX_LARGEST_PBUFFER: %x\n", cur, pop);
      break;

    case WGL_SAMPLE_BUFFERS_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_SAMPLE_BUFFERS_ARB, pop);
      TRACE("pAttr[%d] = GLX_SAMPLE_BUFFERS_ARB: %x\n", cur, pop);
      break;

    case WGL_SAMPLES_ARB:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_SAMPLES_ARB, pop);
      TRACE("pAttr[%d] = GLX_SAMPLES_ARB: %x\n", cur, pop);
      break;

    case WGL_TEXTURE_FORMAT_ARB:
    case WGL_TEXTURE_TARGET_ARB:
    case WGL_MIPMAP_TEXTURE_ARB:
      TRACE("WGL_render_texture Attributes: %x as %x\n", iWGLAttr[cur], iWGLAttr[cur + 1]);
      pop = iWGLAttr[++cur];
      if (NULL == pbuf) {
        ERR("trying to use GLX_Pbuffer Attributes without Pbuffer (was %x)\n", iWGLAttr[cur]);
      }
      if (use_render_texture_ati) {
        /** nothing to do here */
      }
      else if (!use_render_texture_emulation) {
        if (WGL_NO_TEXTURE_ARB != pop) {
          ERR("trying to use WGL_render_texture Attributes without support (was %x)\n", iWGLAttr[cur]);
          return -1; /** error: don't support it */
        } else {
          drawattrib |= GLX_PBUFFER_BIT;
        }
      }
      break ;
    case WGL_FLOAT_COMPONENTS_NV:
      nvfloatattrib = iWGLAttr[++cur];
      TRACE("pAttr[%d] = WGL_FLOAT_COMPONENTS_NV: %x\n", cur, nvfloatattrib);
      break ;
    case WGL_BIND_TO_TEXTURE_DEPTH_NV:
    case WGL_BIND_TO_TEXTURE_RGB_ARB:
    case WGL_BIND_TO_TEXTURE_RGBA_ARB:
    case WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_R_NV:
    case WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RG_NV:
    case WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGB_NV:
    case WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGBA_NV:
      pop = iWGLAttr[++cur];
      /** cannot be converted, see direct handling on 
       *   - wglGetPixelFormatAttribivARB
       *  TODO: wglChoosePixelFormat
       */
      break ;
    case WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT, pop);
      TRACE("pAttr[%d] = GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT: %x\n", cur, pop);
      break ;

    case WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT:
      pop = iWGLAttr[++cur];
      PUSH2(oGLXAttr, GLX_RGBA_UNSIGNED_FLOAT_TYPE_EXT, pop);
      TRACE("pAttr[%d] = GLX_RGBA_UNSIGNED_FLOAT_TYPE_EXT: %x\n", cur, pop);
      break ;
    default:
      FIXME("unsupported %x WGL Attribute\n", iWGLAttr[cur]);
      break;
    }
    ++cur;
  }

  /* By default glXChooseFBConfig defaults to GLX_WINDOW_BIT. wglChoosePixelFormatARB searches through
   * all formats. Unless drawattrib is set to a non-zero value override it with ~0, so that pixmap and pbuffer
   * formats appear as well. */
  if(!drawattrib) drawattrib = ~0;
  PUSH2(oGLXAttr, GLX_DRAWABLE_TYPE, drawattrib);
  TRACE("pAttr[?] = GLX_DRAWABLE_TYPE: %#x\n", drawattrib);

  /* By default glXChooseFBConfig uses GLX_RGBA_BIT as the default value. Since wglChoosePixelFormatARB
   * searches in all formats we have to do the same. For this reason we set GLX_RENDER_TYPE to ~0 unless
   * it is overridden. */
  PUSH2(oGLXAttr, GLX_RENDER_TYPE, pixelattrib);
  TRACE("pAttr[?] = GLX_RENDER_TYPE: %#x\n", pixelattrib);

  /* Set GLX_FLOAT_COMPONENTS_NV all the time */
  if(strstr(WineGLInfo.glxExtensions, "GLX_NV_float_buffer")) {
    PUSH2(oGLXAttr, GLX_FLOAT_COMPONENTS_NV, nvfloatattrib);
    TRACE("pAttr[?] = GLX_FLOAT_COMPONENTS_NV: %#x\n", nvfloatattrib);
  }

  return nAttribs;
}

static int get_render_type_from_fbconfig(Display *display, GLXFBConfig fbconfig)
{
    int render_type=0, render_type_bit;
    pglXGetFBConfigAttrib(display, fbconfig, GLX_RENDER_TYPE, &render_type_bit);
    switch(render_type_bit)
    {
        case GLX_RGBA_BIT:
            render_type = GLX_RGBA_TYPE;
            break;
        case GLX_COLOR_INDEX_BIT:
            render_type = GLX_COLOR_INDEX_TYPE;
            break;
        case GLX_RGBA_FLOAT_BIT:
            render_type = GLX_RGBA_FLOAT_TYPE;
            break;
        case GLX_RGBA_UNSIGNED_FLOAT_BIT_EXT:
            render_type = GLX_RGBA_UNSIGNED_FLOAT_TYPE_EXT;
            break;
        default:
            ERR("Unknown render_type: %x\n", render_type_bit);
    }
    return render_type;
}

/* Check whether a fbconfig is suitable for Windows-style bitmap rendering */
static BOOL check_fbconfig_bitmap_capability(Display *display, GLXFBConfig fbconfig)
{
    int dbuf, value;
    pglXGetFBConfigAttrib(display, fbconfig, GLX_DOUBLEBUFFER, &dbuf);
    pglXGetFBConfigAttrib(gdi_display, fbconfig, GLX_DRAWABLE_TYPE, &value);

    /* Windows only supports bitmap rendering on single buffered formats, further the fbconfig needs to have
     * the GLX_PIXMAP_BIT set. */
    return !dbuf && (value & GLX_PIXMAP_BIT);
}

static WineGLPixelFormat *get_formats(Display *display, int *size_ret, int *onscreen_size_ret)
{
    static WineGLPixelFormat *list;
    static int size, onscreen_size;

    int fmt_id, nCfgs, i, run, bmp_formats;
    GLXFBConfig* cfgs;
    XVisualInfo *visinfo;

    wine_tsx11_lock();
    if (list) goto done;

    cfgs = pglXGetFBConfigs(display, DefaultScreen(display), &nCfgs);
    if (NULL == cfgs || 0 == nCfgs) {
        if(cfgs != NULL) XFree(cfgs);
        wine_tsx11_unlock();
        ERR("glXChooseFBConfig returns NULL\n");
        return NULL;
    }

    /* Bitmap rendering on Windows implies the use of the Microsoft GDI software renderer.
     * Further most GLX drivers only offer pixmap rendering using indirect rendering (except for modern drivers which support 'AIGLX' / composite).
     * Indirect rendering can indicate software rendering (on Nvidia it is hw accelerated)
     * Since bitmap rendering implies the use of software rendering we can safely use indirect rendering for bitmaps.
     *
     * Below we count the number of formats which are suitable for bitmap rendering. Windows restricts bitmap rendering to single buffered formats.
     */
    for(i=0, bmp_formats=0; i<nCfgs; i++)
    {
        if(check_fbconfig_bitmap_capability(display, cfgs[i]))
            bmp_formats++;
    }
    TRACE("Found %d bitmap capable fbconfigs\n", bmp_formats);

    list = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (nCfgs + bmp_formats)*sizeof(WineGLPixelFormat));

    /* Fill the pixel format list. Put onscreen formats at the top and offscreen ones at the bottom.
     * Do this as GLX doesn't guarantee that the list is sorted */
    for(run=0; run < 2; run++)
    {
        for(i=0; i<nCfgs; i++) {
            pglXGetFBConfigAttrib(display, cfgs[i], GLX_FBCONFIG_ID, &fmt_id);
            visinfo = pglXGetVisualFromFBConfig(display, cfgs[i]);

            /* The first run we only add onscreen formats (ones which have an associated X Visual).
             * The second run we only set offscreen formats. */
            if(!run && visinfo)
            {
                /* We implement child window rendering using offscreen buffers (using composite or an XPixmap).
                 * The contents is copied to the destination using XCopyArea. For the copying to work
                 * the depth of the source and destination window should be the same. In general this should
                 * not be a problem for OpenGL as drivers only advertise formats with a similar depth (or no depth).
                 * As of the introduction of composition managers at least Nvidia now also offers ARGB visuals
                 * with a depth of 32 in addition to the default 24 bit. In order to prevent BadMatch errors we only
                 * list formats with the same depth. */
                if(visinfo->depth != screen_depth)
                {
                    XFree(visinfo);
                    continue;
                }

                TRACE("Found onscreen format FBCONFIG_ID 0x%x corresponding to iPixelFormat %d at GLX index %d\n", fmt_id, size+1, i);
                list[size].iPixelFormat = size+1; /* The index starts at 1 */
                list[size].fbconfig = cfgs[i];
                list[size].fmt_id = fmt_id;
                list[size].render_type = get_render_type_from_fbconfig(display, cfgs[i]);
                list[size].offscreenOnly = FALSE;
                list[size].dwFlags = 0;
                size++;
                onscreen_size++;

                /* Clone a format if it is bitmap capable for indirect rendering to bitmaps */
                if(check_fbconfig_bitmap_capability(display, cfgs[i]))
                {
                    TRACE("Found bitmap capable format FBCONFIG_ID 0x%x corresponding to iPixelFormat %d at GLX index %d\n", fmt_id, size+1, i);
                    list[size].iPixelFormat = size+1; /* The index starts at 1 */
                    list[size].fbconfig = cfgs[i];
                    list[size].fmt_id = fmt_id;
                    list[size].render_type = get_render_type_from_fbconfig(display, cfgs[i]);
                    list[size].offscreenOnly = FALSE;
                    list[size].dwFlags = PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_GENERIC_FORMAT;
                    size++;
                    onscreen_size++;
                }
            } else if(run && !visinfo) {
                int window_drawable=0;
                pglXGetFBConfigAttrib(gdi_display, cfgs[i], GLX_DRAWABLE_TYPE, &window_drawable);

                /* Recent Nvidia drivers and DRI drivers offer window drawable formats without a visual.
                 * This are formats like 16-bit rgb on a 24-bit desktop. In order to support these formats
                 * onscreen we would have to use glXCreateWindow instead of XCreateWindow. Further it will
                 * likely make our child window opengl rendering more complicated since likely you can't use
                 * XCopyArea on a GLX Window.
                 * For now ignore fbconfigs which are window drawable but lack a visual. */
                if(window_drawable & GLX_WINDOW_BIT)
                {
                    TRACE("Skipping FBCONFIG_ID 0x%x as an offscreen format because it is window_drawable\n", fmt_id);
                    continue;
                }

                TRACE("Found offscreen format FBCONFIG_ID 0x%x corresponding to iPixelFormat %d at GLX index %d\n", fmt_id, size+1, i);
                list[size].iPixelFormat = size+1; /* The index starts at 1 */
                list[size].fbconfig = cfgs[i];
                list[size].fmt_id = fmt_id;
                list[size].render_type = get_render_type_from_fbconfig(display, cfgs[i]);
                list[size].offscreenOnly = TRUE;
                list[size].dwFlags = 0;
                size++;
            }

            if (visinfo) XFree(visinfo);
        }
    }

    XFree(cfgs);

done:
    if (size_ret) *size_ret = size;
    if (onscreen_size_ret) *onscreen_size_ret = onscreen_size;
    wine_tsx11_unlock();
    return list;
}

/* GLX can advertise dozens of different pixelformats including offscreen and onscreen ones.
 * In our WGL implementation we only support a subset of these formats namely the format of
 * Wine's main visual and offscreen formats (if they are available).
 * This function converts a WGL format to its corresponding GLX one. It returns a WineGLPixelFormat
 * and it returns the number of supported WGL formats in fmt_count.
 */
static WineGLPixelFormat* ConvertPixelFormatWGLtoGLX(Display *display, int iPixelFormat, BOOL AllowOffscreen, int *fmt_count)
{
    WineGLPixelFormat *list, *res = NULL;
    int size, onscreen_size;

    if (!(list = get_formats(display, &size, &onscreen_size ))) return NULL;

    /* Check if the pixelformat is valid. Note that it is legal to pass an invalid
     * iPixelFormat in case of probing the number of pixelformats.
     */
    if((iPixelFormat > 0) && (iPixelFormat <= size) &&
       (!list[iPixelFormat-1].offscreenOnly || AllowOffscreen)) {
        res = &list[iPixelFormat-1];
        TRACE("Returning fmt_id=%#x for iPixelFormat=%d\n", res->fmt_id, iPixelFormat);
    }

    if(AllowOffscreen)
        *fmt_count = size;
    else
        *fmt_count = onscreen_size;

    TRACE("Number of returned pixelformats=%d\n", *fmt_count);

    return res;
}

/* Search our internal pixelformat list for the WGL format corresponding to the given fbconfig */
static WineGLPixelFormat* ConvertPixelFormatGLXtoWGL(Display *display, int fmt_id, DWORD dwFlags)
{
    WineGLPixelFormat *list;
    int i, size;

    if (!(list = get_formats(display, &size, NULL ))) return NULL;

    for(i=0; i<size; i++) {
        /* A GLX format can appear multiple times in the pixel format list due to fake formats for bitmap rendering.
         * Fake formats might get selected when the user passes the proper flags using the dwFlags parameter. */
        if( (list[i].fmt_id == fmt_id) && ((list[i].dwFlags & dwFlags) == dwFlags) ) {
            TRACE("Returning iPixelFormat %d for fmt_id 0x%x\n", list[i].iPixelFormat, fmt_id);
            return &list[i];
        }
    }
    TRACE("No compatible format found for fmt_id 0x%x\n", fmt_id);
    return NULL;
}

int pixelformat_from_fbconfig_id(XID fbconfig_id)
{
    WineGLPixelFormat *fmt;

    if (!fbconfig_id) return 0;

    fmt = ConvertPixelFormatGLXtoWGL(gdi_display, fbconfig_id, 0 /* no flags */);
    if(fmt)
        return fmt->iPixelFormat;
    /* This will happen on hwnds without a pixel format set; it's ok */
    return 0;
}


/* Mark any allocated context using the glx drawable 'old' to use 'new' */
void mark_drawable_dirty(Drawable old, Drawable new)
{
    Wine_GLContext *ctx;
    for (ctx = context_list; ctx; ctx = ctx->next) {
        if (old == ctx->drawables[0]) {
            ctx->drawables[0] = new;
            ctx->refresh_drawables = TRUE;
        }
        if (old == ctx->drawables[1]) {
            ctx->drawables[1] = new;
            ctx->refresh_drawables = TRUE;
        }
    }
}

/* Given the current context, make sure its drawable is sync'd */
static inline void sync_context(Wine_GLContext *context)
{
    if(context && context->refresh_drawables) {
        if (glxRequireVersion(3))
            pglXMakeContextCurrent(gdi_display, context->drawables[0],
                                   context->drawables[1], context->ctx);
        else
            pglXMakeCurrent(gdi_display, context->drawables[0], context->ctx);
        context->refresh_drawables = FALSE;
    }
}


static GLXContext create_glxcontext(Display *display, Wine_GLContext *context, GLXContext shareList)
{
    GLXContext ctx;

    /* We use indirect rendering for rendering to bitmaps. See get_formats for a comment about this. */
    BOOL indirect = (context->fmt->dwFlags & PFD_DRAW_TO_BITMAP) ? FALSE : TRUE;

    if(context->gl3_context)
    {
        if(context->numAttribs)
            ctx = pglXCreateContextAttribsARB(gdi_display, context->fmt->fbconfig, shareList, indirect, context->attribList);
        else
            ctx = pglXCreateContextAttribsARB(gdi_display, context->fmt->fbconfig, shareList, indirect, NULL);
    }
    else if(context->vis)
        ctx = pglXCreateContext(gdi_display, context->vis, shareList, indirect);
    else /* Create a GLX Context for a pbuffer */
        ctx = pglXCreateNewContext(gdi_display, context->fmt->fbconfig, context->fmt->render_type, shareList, TRUE);

    return ctx;
}


Drawable create_glxpixmap(Display *display, XVisualInfo *vis, Pixmap parent)
{
    return pglXCreateGLXPixmap(display, vis, parent);
}


static XID create_bitmap_glxpixmap(X11DRV_PDEVICE *physDev, WineGLPixelFormat *fmt)
{
    GLXPixmap ret = 0;
    XVisualInfo *vis;

    wine_tsx11_lock();

    vis = pglXGetVisualFromFBConfig(gdi_display, fmt->fbconfig);
    if(vis) {
        if(vis->depth == physDev->bitmap->pixmap_depth)
            ret = pglXCreateGLXPixmap(gdi_display, vis, physDev->bitmap->pixmap);
        XFree(vis);
    }
    wine_tsx11_unlock();
    TRACE("return %lx\n", ret);
    return ret;
}

/**
 * X11DRV_ChoosePixelFormat
 *
 * Equivalent to glXChooseVisual.
 */
int CDECL X11DRV_ChoosePixelFormat(X11DRV_PDEVICE *physDev,
			     const PIXELFORMATDESCRIPTOR *ppfd) {
    WineGLPixelFormat *list;
    int onscreen_size;
    int ret = 0;
    int value = 0;
    int i = 0;
    int bestFormat = -1;
    int bestDBuffer = -1;
    int bestStereo = -1;
    int bestColor = -1;
    int bestAlpha = -1;
    int bestDepth = -1;
    int bestStencil = -1;
    int bestAux = -1;

    if (!has_opengl()) return 0;

    if (TRACE_ON(wgl)) {
        TRACE("(%p,%p)\n", physDev, ppfd);

        dump_PIXELFORMATDESCRIPTOR(ppfd);
    }

    if (!(list = get_formats(gdi_display, NULL, &onscreen_size ))) return 0;

    wine_tsx11_lock();
    for(i=0; i<onscreen_size; i++)
    {
        int dwFlags = 0;
        int iPixelType = 0;
        int alpha=0, color=0, depth=0, stencil=0, aux=0;
        WineGLPixelFormat *fmt = &list[i];

        /* Pixel type */
        pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_RENDER_TYPE, &value);
        if (value & GLX_RGBA_BIT)
            iPixelType = PFD_TYPE_RGBA;
        else
            iPixelType = PFD_TYPE_COLORINDEX;

        if (ppfd->iPixelType != iPixelType)
        {
            TRACE("pixel type mismatch for iPixelFormat=%d\n", i+1);
            continue;
        }

        /* Only use bitmap capable for formats for bitmap rendering.
         * See get_formats for more info. */
        if( (ppfd->dwFlags & PFD_DRAW_TO_BITMAP) != (fmt->dwFlags & PFD_DRAW_TO_BITMAP))
        {
            TRACE("PFD_DRAW_TO_BITMAP mismatch for iPixelFormat=%d\n", i+1);
            continue;
        }

        pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_DOUBLEBUFFER, &value);
        if (value) dwFlags |= PFD_DOUBLEBUFFER;
        pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_STEREO, &value);
        if (value) dwFlags |= PFD_STEREO;
        pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_BUFFER_SIZE, &color); /* cColorBits */
        pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_ALPHA_SIZE, &alpha); /* cAlphaBits */
        pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_DEPTH_SIZE, &depth); /* cDepthBits */
        pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_STENCIL_SIZE, &stencil); /* cStencilBits */
        pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_AUX_BUFFERS, &aux); /* cAuxBuffers */

        /* The behavior of PDF_STEREO/PFD_STEREO_DONTCARE and PFD_DOUBLEBUFFER / PFD_DOUBLEBUFFER_DONTCARE
         * is not very clear on MSDN. They specify that ChoosePixelFormat tries to match pixel formats
         * with the flag (PFD_STEREO / PFD_DOUBLEBUFFERING) set. Otherwise it says that it tries to match
         * formats without the given flag set.
         * A test on Windows using a Radeon 9500pro on WinXP (the driver doesn't support Stereo)
         * has indicated that a format without stereo is returned when stereo is unavailable.
         * So in case PFD_STEREO is set, formats that support it should have priority above formats
         * without. In case PFD_STEREO_DONTCARE is set, stereo is ignored.
         *
         * To summarize the following is most likely the correct behavior:
         * stereo not set -> prefer no-stereo formats, else also accept stereo formats
         * stereo set -> prefer stereo formats, else also accept no-stereo formats
         * stereo don't care -> it doesn't matter whether we get stereo or not
         *
         * In Wine we will treat no-stereo the same way as don't care because it makes
         * format selection even more complicated and second drivers with Stereo advertise
         * each format twice anyway.
         */

        /* Doublebuffer, see the comments above */
        if( !(ppfd->dwFlags & PFD_DOUBLEBUFFER_DONTCARE) ) {
            if( ((ppfd->dwFlags & PFD_DOUBLEBUFFER) != bestDBuffer) &&
                ((dwFlags & PFD_DOUBLEBUFFER) == (ppfd->dwFlags & PFD_DOUBLEBUFFER)) )
            {
                bestDBuffer = dwFlags & PFD_DOUBLEBUFFER;
                bestStereo = dwFlags & PFD_STEREO;
                bestAlpha = alpha;
                bestColor = color;
                bestDepth = depth;
                bestStencil = stencil;
                bestAux = aux;
                bestFormat = i;
                continue;
            }
            if(bestDBuffer != -1 && (dwFlags & PFD_DOUBLEBUFFER) != bestDBuffer)
                continue;
        }

        /* Stereo, see the comments above. */
        if( !(ppfd->dwFlags & PFD_STEREO_DONTCARE) ) {
            if( ((ppfd->dwFlags & PFD_STEREO) != bestStereo) &&
                ((dwFlags & PFD_STEREO) == (ppfd->dwFlags & PFD_STEREO)) )
            {
                bestDBuffer = dwFlags & PFD_DOUBLEBUFFER;
                bestStereo = dwFlags & PFD_STEREO;
                bestAlpha = alpha;
                bestColor = color;
                bestDepth = depth;
                bestStencil = stencil;
                bestAux = aux;
                bestFormat = i;
                continue;
            }
            if(bestStereo != -1 && (dwFlags & PFD_STEREO) != bestStereo)
                continue;
        }

        /* Below we will do a number of checks to select the 'best' pixelformat.
         * We assume the precedence cColorBits > cAlphaBits > cDepthBits > cStencilBits -> cAuxBuffers.
         * The code works by trying to match the most important options as close as possible.
         * When a reasonable format is found, we will try to match more options.
         * It appears (see the opengl32 test) that Windows opengl drivers ignore options
         * like cColorBits, cAlphaBits and friends if they are set to 0, so they are considered
         * as DONTCARE. At least Serious Sam TSE relies on this behavior. */

        /* Color bits */
        if(ppfd->cColorBits) {
            if( ((ppfd->cColorBits > bestColor) && (color > bestColor)) ||
                ((color >= ppfd->cColorBits) && (color < bestColor)) )
            {
                bestDBuffer = dwFlags & PFD_DOUBLEBUFFER;
                bestStereo = dwFlags & PFD_STEREO;
                bestAlpha = alpha;
                bestColor = color;
                bestDepth = depth;
                bestStencil = stencil;
                bestAux = aux;
                bestFormat = i;
                continue;
            } else if(bestColor != color) {  /* Do further checks if the format is compatible */
                TRACE("color mismatch for iPixelFormat=%d\n", i+1);
                continue;
            }
        }

        /* Alpha bits */
        if(ppfd->cAlphaBits) {
            if( ((ppfd->cAlphaBits > bestAlpha) && (alpha > bestAlpha)) ||
                ((alpha >= ppfd->cAlphaBits) && (alpha < bestAlpha)) )
            {
                bestDBuffer = dwFlags & PFD_DOUBLEBUFFER;
                bestStereo = dwFlags & PFD_STEREO;
                bestAlpha = alpha;
                bestColor = color;
                bestDepth = depth;
                bestStencil = stencil;
                bestAux = aux;
                bestFormat = i;
                continue;
            } else if(bestAlpha != alpha) {
                TRACE("alpha mismatch for iPixelFormat=%d\n", i+1);
                continue;
            }
        }

        /* Depth bits */
        if(ppfd->cDepthBits) {
            if( ((ppfd->cDepthBits > bestDepth) && (depth > bestDepth)) ||
                ((depth >= ppfd->cDepthBits) && (depth < bestDepth)) )
            {
                bestDBuffer = dwFlags & PFD_DOUBLEBUFFER;
                bestStereo = dwFlags & PFD_STEREO;
                bestAlpha = alpha;
                bestColor = color;
                bestDepth = depth;
                bestStencil = stencil;
                bestAux = aux;
                bestFormat = i;
                continue;
            } else if(bestDepth != depth) {
                TRACE("depth mismatch for iPixelFormat=%d\n", i+1);
                continue;
            }
        }

        /* Stencil bits */
        if(ppfd->cStencilBits) {
            if( ((ppfd->cStencilBits > bestStencil) && (stencil > bestStencil)) ||
                ((stencil >= ppfd->cStencilBits) && (stencil < bestStencil)) )
            {
                bestDBuffer = dwFlags & PFD_DOUBLEBUFFER;
                bestStereo = dwFlags & PFD_STEREO;
                bestAlpha = alpha;
                bestColor = color;
                bestDepth = depth;
                bestStencil = stencil;
                bestAux = aux;
                bestFormat = i;
                continue;
            } else if(bestStencil != stencil) {
                TRACE("stencil mismatch for iPixelFormat=%d\n", i+1);
                continue;
            }
        }

        /* Aux buffers */
        if(ppfd->cAuxBuffers) {
            if( ((ppfd->cAuxBuffers > bestAux) && (aux > bestAux)) ||
                ((aux >= ppfd->cAuxBuffers) && (aux < bestAux)) )
            {
                bestDBuffer = dwFlags & PFD_DOUBLEBUFFER;
                bestStereo = dwFlags & PFD_STEREO;
                bestAlpha = alpha;
                bestColor = color;
                bestDepth = depth;
                bestStencil = stencil;
                bestAux = aux;
                bestFormat = i;
                continue;
            } else if(bestAux != aux) {
                TRACE("aux mismatch for iPixelFormat=%d\n", i+1);
                continue;
            }
        }
    }

    if(bestFormat == -1) {
        TRACE("No matching mode was found returning 0\n");
        ret = 0;
    }
    else {
        ret = bestFormat+1; /* the return value should be a 1-based index */
        TRACE("Successfully found a matching mode, returning index: %d %x\n", ret, list[bestFormat].fmt_id);
    }

    wine_tsx11_unlock();

    return ret;
}
/**
 * X11DRV_DescribePixelFormat
 *
 * Get the pixel-format descriptor associated to the given id
 */
int CDECL X11DRV_DescribePixelFormat(X11DRV_PDEVICE *physDev,
			       int iPixelFormat,
			       UINT nBytes,
			       PIXELFORMATDESCRIPTOR *ppfd) {
  /*XVisualInfo *vis;*/
  int value;
  int rb,gb,bb,ab;
  WineGLPixelFormat *fmt;
  int ret = 0;
  int fmt_count = 0;

  if (!has_opengl()) return 0;

  TRACE("(%p,%d,%d,%p)\n", physDev, iPixelFormat, nBytes, ppfd);

  /* Look for the iPixelFormat in our list of supported formats. If it is supported we get the index in the FBConfig table and the number of supported formats back */
  fmt = ConvertPixelFormatWGLtoGLX(gdi_display, iPixelFormat, FALSE /* Offscreen */, &fmt_count);
  if (ppfd == NULL) {
      /* The application is only querying the number of pixelformats */
      return fmt_count;
  } else if(fmt == NULL) {
      WARN("unexpected iPixelFormat(%d): not >=1 and <=nFormats(%d), returning NULL!\n", iPixelFormat, fmt_count);
      return 0;
  }

  if (nBytes < sizeof(PIXELFORMATDESCRIPTOR)) {
    ERR("Wrong structure size !\n");
    /* Should set error */
    return 0;
  }

  ret = fmt_count;

  memset(ppfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
  ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
  ppfd->nVersion = 1;

  /* These flags are always the same... */
  ppfd->dwFlags = PFD_SUPPORT_OPENGL;
  /* Now the flags extracted from the Visual */

  wine_tsx11_lock();

  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_DRAWABLE_TYPE, &value);
  if(value & GLX_WINDOW_BIT)
      ppfd->dwFlags |= PFD_DRAW_TO_WINDOW;

  /* On Windows bitmap rendering is only offered using the GDI Software renderer. We reserve some formats (see get_formats for more info)
   * for bitmap rendering since we require indirect rendering for this. Further pixel format logs of a GeforceFX, Geforce8800GT, Radeon HD3400 and a
   * Radeon 9000 indicated that all bitmap formats have PFD_SUPPORT_GDI. Except for 2 formats on the Radeon 9000 none of the hw accelerated formats
   * offered the GDI bit either. */
  ppfd->dwFlags |= fmt->dwFlags & (PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI);

  /* PFD_GENERIC_FORMAT - gdi software rendering
   * PFD_GENERIC_ACCELERATED - some parts are accelerated by a display driver (MCD e.g. 3dfx minigl)
   * none set - full hardware accelerated by a ICD
   *
   * We only set PFD_GENERIC_FORMAT on bitmap formats (see get_formats) as that's what ATI and Nvidia Windows drivers do  */
  ppfd->dwFlags |= fmt->dwFlags & (PFD_GENERIC_FORMAT | PFD_GENERIC_ACCELERATED);

  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_DOUBLEBUFFER, &value);
  if (value) {
      ppfd->dwFlags |= PFD_DOUBLEBUFFER;
      ppfd->dwFlags &= ~PFD_SUPPORT_GDI;
  }
  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_STEREO, &value); if (value) ppfd->dwFlags |= PFD_STEREO;

  /* Pixel type */
  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_RENDER_TYPE, &value);
  if (value & GLX_RGBA_BIT)
    ppfd->iPixelType = PFD_TYPE_RGBA;
  else
    ppfd->iPixelType = PFD_TYPE_COLORINDEX;

  /* Color bits */
  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_BUFFER_SIZE, &value);
  ppfd->cColorBits = value;

  /* Red, green, blue and alpha bits / shifts */
  if (ppfd->iPixelType == PFD_TYPE_RGBA) {
    pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_RED_SIZE, &rb);
    pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_GREEN_SIZE, &gb);
    pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_BLUE_SIZE, &bb);
    pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_ALPHA_SIZE, &ab);

    ppfd->cRedBits = rb;
    ppfd->cRedShift = gb + bb + ab;
    ppfd->cBlueBits = bb;
    ppfd->cBlueShift = ab;
    ppfd->cGreenBits = gb;
    ppfd->cGreenShift = bb + ab;
    ppfd->cAlphaBits = ab;
    ppfd->cAlphaShift = 0;
  } else {
    ppfd->cRedBits = 0;
    ppfd->cRedShift = 0;
    ppfd->cBlueBits = 0;
    ppfd->cBlueShift = 0;
    ppfd->cGreenBits = 0;
    ppfd->cGreenShift = 0;
    ppfd->cAlphaBits = 0;
    ppfd->cAlphaShift = 0;
  }

  /* Accum RGBA bits */
  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_ACCUM_RED_SIZE, &rb);
  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_ACCUM_GREEN_SIZE, &gb);
  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_ACCUM_BLUE_SIZE, &bb);
  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_ACCUM_ALPHA_SIZE, &ab);

  ppfd->cAccumBits = rb+gb+bb+ab;
  ppfd->cAccumRedBits = rb;
  ppfd->cAccumGreenBits = gb;
  ppfd->cAccumBlueBits = bb;
  ppfd->cAccumAlphaBits = ab;

  /* Aux bits */
  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_AUX_BUFFERS, &value);
  ppfd->cAuxBuffers = value;

  /* Depth bits */
  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_DEPTH_SIZE, &value);
  ppfd->cDepthBits = value;

  /* stencil bits */
  pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_STENCIL_SIZE, &value);
  ppfd->cStencilBits = value;

  wine_tsx11_unlock();

  ppfd->iLayerType = PFD_MAIN_PLANE;

  if (TRACE_ON(wgl)) {
    dump_PIXELFORMATDESCRIPTOR(ppfd);
  }

  return ret;
}

/**
 * X11DRV_GetPixelFormat
 *
 * Get the pixel-format id used by this DC
 */
int CDECL X11DRV_GetPixelFormat(X11DRV_PDEVICE *physDev) {
  WineGLPixelFormat *fmt;
  int tmp;
  TRACE("(%p)\n", physDev);

  if (!physDev->current_pf) return 0;  /* not set yet */

  fmt = ConvertPixelFormatWGLtoGLX(gdi_display, physDev->current_pf, TRUE, &tmp);
  if(!fmt)
  {
    ERR("Unable to find a WineGLPixelFormat for iPixelFormat=%d\n", physDev->current_pf);
    return 0;
  }
  else if(fmt->offscreenOnly)
  {
    /* Offscreen formats can't be used with traditional WGL calls.
     * As has been verified on Windows GetPixelFormat doesn't fail but returns iPixelFormat=1. */
     TRACE("Returning iPixelFormat=1 for offscreen format: %d\n", fmt->iPixelFormat);
    return 1;
  }

  TRACE("(%p): returns %d\n", physDev, physDev->current_pf);
  return physDev->current_pf;
}

/* This function is the core of X11DRV_SetPixelFormat and X11DRV_SetPixelFormatWINE.
 * Both functions are the same except that X11DRV_SetPixelFormatWINE allows you to
 * set the pixel format multiple times. */
static BOOL internal_SetPixelFormat(X11DRV_PDEVICE *physDev,
			   int iPixelFormat,
			   const PIXELFORMATDESCRIPTOR *ppfd) {
    WineGLPixelFormat *fmt;
    int value;
    HWND hwnd;

    /* SetPixelFormat is not allowed on the X root_window e.g. GetDC(0) */
    if(get_glxdrawable(physDev) == root_window)
    {
        ERR("Invalid operation on root_window\n");
        return FALSE;
    }

    /* Check if iPixelFormat is in our list of supported formats to see if it is supported. */
    fmt = ConvertPixelFormatWGLtoGLX(gdi_display, iPixelFormat, FALSE /* Offscreen */, &value);
    if(!fmt) {
        ERR("Invalid iPixelFormat: %d\n", iPixelFormat);
        return FALSE;
    }

    wine_tsx11_lock();
    pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_DRAWABLE_TYPE, &value);
    wine_tsx11_unlock();

    hwnd = WindowFromDC(physDev->hdc);
    if(hwnd) {
        if(!(value&GLX_WINDOW_BIT)) {
            WARN("Pixel format %d is not compatible for window rendering\n", iPixelFormat);
            return FALSE;
        }

        if(!SendMessageW(hwnd, WM_X11DRV_SET_WIN_FORMAT, fmt->fmt_id, 0)) {
            ERR("Couldn't set format of the window, returning failure\n");
            return FALSE;
        }
    }
    else if(physDev->bitmap) {
        if(!(value&GLX_PIXMAP_BIT)) {
            WARN("Pixel format %d is not compatible for bitmap rendering\n", iPixelFormat);
            return FALSE;
        }

        physDev->bitmap->glxpixmap = create_bitmap_glxpixmap(physDev, fmt);
        if(!physDev->bitmap->glxpixmap) {
            WARN("Couldn't create glxpixmap for pixel format %d\n", iPixelFormat);
            return FALSE;
        }
    }
    else {
        FIXME("called on a non-window, non-bitmap object?\n");
    }

    physDev->current_pf = iPixelFormat;

    if (TRACE_ON(wgl)) {
        int gl_test = 0;

        wine_tsx11_lock();
        gl_test = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_FBCONFIG_ID, &value);
        if (gl_test) {
           ERR("Failed to retrieve FBCONFIG_ID from GLXFBConfig, expect problems.\n");
        } else {
            TRACE(" FBConfig have :\n");
            TRACE(" - FBCONFIG_ID   0x%x\n", value);
            pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_VISUAL_ID, &value);
            TRACE(" - VISUAL_ID     0x%x\n", value);
            pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_DRAWABLE_TYPE, &value);
            TRACE(" - DRAWABLE_TYPE 0x%x\n", value);
        }
        wine_tsx11_unlock();
    }
    return TRUE;
}


/**
 * X11DRV_SetPixelFormat
 *
 * Set the pixel-format id used by this DC
 */
BOOL CDECL X11DRV_SetPixelFormat(X11DRV_PDEVICE *physDev,
			   int iPixelFormat,
			   const PIXELFORMATDESCRIPTOR *ppfd) {
    TRACE("(%p,%d,%p)\n", physDev, iPixelFormat, ppfd);

    if (!has_opengl()) return FALSE;

    if(physDev->current_pf)  /* cannot change it if already set */
        return (physDev->current_pf == iPixelFormat);

    return internal_SetPixelFormat(physDev, iPixelFormat, ppfd);
}

/**
 * X11DRV_wglCopyContext
 *
 * For OpenGL32 wglCopyContext.
 */
BOOL CDECL X11DRV_wglCopyContext(HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask) {
    Wine_GLContext *src = (Wine_GLContext*)hglrcSrc;
    Wine_GLContext *dst = (Wine_GLContext*)hglrcDst;

    TRACE("hglrcSrc: (%p), hglrcDst: (%p), mask: %#x\n", hglrcSrc, hglrcDst, mask);

    wine_tsx11_lock();
    pglXCopyContext(gdi_display, src->ctx, dst->ctx, mask);
    wine_tsx11_unlock();

    /* As opposed to wglCopyContext, glXCopyContext doesn't return anything, so hopefully we passed */
    return TRUE;
}

/**
 * X11DRV_wglCreateContext
 *
 * For OpenGL32 wglCreateContext.
 */
HGLRC CDECL X11DRV_wglCreateContext(X11DRV_PDEVICE *physDev)
{
    Wine_GLContext *ret;
    WineGLPixelFormat *fmt;
    int hdcPF = physDev->current_pf;
    int fmt_count = 0;
    HDC hdc = physDev->hdc;

    TRACE("(%p)->(PF:%d)\n", hdc, hdcPF);

    if (!has_opengl()) return 0;

    fmt = ConvertPixelFormatWGLtoGLX(gdi_display, hdcPF, TRUE /* Offscreen */, &fmt_count);
    /* We can render using the iPixelFormat (1) of Wine's Main visual AND using some offscreen formats.
     * Note that standard WGL-calls don't recognize offscreen-only formats. For that reason pbuffers
     * use a sort of 'proxy' HDC (wglGetPbufferDCARB).
     * If this fails something is very wrong on the system. */
    if(!fmt) {
        ERR("Cannot get FB Config for iPixelFormat %d, expect problems!\n", hdcPF);
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return NULL;
    }

    wine_tsx11_lock();
    ret = alloc_context();
    ret->hdc = hdc;
    ret->fmt = fmt;
    ret->has_been_current = FALSE;
    ret->sharing = FALSE;

    ret->vis = pglXGetVisualFromFBConfig(gdi_display, fmt->fbconfig);
    ret->ctx = create_glxcontext(gdi_display, ret, NULL);
    wine_tsx11_unlock();

    TRACE(" creating context %p (GL context creation delayed)\n", ret);
    return (HGLRC) ret;
}

/**
 * X11DRV_wglDeleteContext
 *
 * For OpenGL32 wglDeleteContext.
 */
BOOL CDECL X11DRV_wglDeleteContext(HGLRC hglrc)
{
    Wine_GLContext *ctx = (Wine_GLContext *) hglrc;

    TRACE("(%p)\n", hglrc);

    if (!has_opengl()) return 0;

    if (!is_valid_context(ctx))
    {
        WARN("Error deleting context !\n");
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    /* WGL doesn't allow deletion of a context which is current in another thread */
    if (ctx->tid != 0 && ctx->tid != GetCurrentThreadId())
    {
        TRACE("Cannot delete context=%p because it is current in another thread.\n", ctx);
        return FALSE;
    }

    /* WGL makes a context not current if it is active before deletion. GLX waits until the context is not current. */
    if (ctx == NtCurrentTeb()->glContext)
        wglMakeCurrent(ctx->hdc, NULL);

    if (ctx->ctx)
    {
        wine_tsx11_lock();
        pglXDestroyContext(gdi_display, ctx->ctx);
        wine_tsx11_unlock();
    }

    return TRUE;
}

/**
 * X11DRV_wglGetCurrentReadDCARB
 *
 * For OpenGL32 wglGetCurrentReadDCARB.
 */
static HDC WINAPI X11DRV_wglGetCurrentReadDCARB(void) 
{
    HDC ret = 0;
    Wine_GLContext *ctx = NtCurrentTeb()->glContext;

    if (ctx) ret = ctx->read_hdc;

    TRACE(" returning %p (GL drawable %lu)\n", ret, ctx ? ctx->drawables[1] : 0);
    return ret;
}

/**
 * X11DRV_wglGetProcAddress
 *
 * For OpenGL32 wglGetProcAddress.
 */
PROC CDECL X11DRV_wglGetProcAddress(LPCSTR lpszProc)
{
    int i, j;
    const WineGLExtension *ext;

    int padding = 32 - strlen(lpszProc);
    if (padding < 0)
        padding = 0;

    if (!has_opengl()) return NULL;

    /* Check the table of WGL extensions to see if we need to return a WGL extension
     * or a function pointer to a native OpenGL function. */
    if(strncmp(lpszProc, "wgl", 3) != 0) {
        return pglXGetProcAddressARB((const GLubyte*)lpszProc);
    } else {
        TRACE("('%s'):%*s", lpszProc, padding, " ");
        for (i = 0; i < WineGLExtensionListSize; ++i) {
            ext = WineGLExtensionList[i];
            for (j = 0; ext->extEntryPoints[j].funcName; ++j) {
                if (strcmp(ext->extEntryPoints[j].funcName, lpszProc) == 0) {
                    TRACE("(%p) - WineGL\n", ext->extEntryPoints[j].funcAddress);
                    return ext->extEntryPoints[j].funcAddress;
                }
            }
        }
    }

    WARN("(%s) - not found\n", lpszProc);
    return NULL;
}

/**
 * X11DRV_wglMakeCurrent
 *
 * For OpenGL32 wglMakeCurrent.
 */
BOOL CDECL X11DRV_wglMakeCurrent(X11DRV_PDEVICE *physDev, HGLRC hglrc) {
    BOOL ret;
    HDC hdc = physDev->hdc;
    DWORD type = GetObjectType(hdc);
    Wine_GLContext *ctx = (Wine_GLContext *) hglrc;

    TRACE("(%p,%p)\n", hdc, hglrc);

    if (!has_opengl()) return FALSE;

    wine_tsx11_lock();
    if (hglrc == NULL)
    {
        Wine_GLContext *prev_ctx = NtCurrentTeb()->glContext;
        if (prev_ctx) prev_ctx->tid = 0;

        ret = pglXMakeCurrent(gdi_display, None, NULL);
        NtCurrentTeb()->glContext = NULL;
    }
    else if (ctx->fmt->iPixelFormat != physDev->current_pf)
    {
        WARN( "mismatched pixel format hdc %p %u ctx %p %u\n",
              hdc, physDev->current_pf, ctx, ctx->fmt->iPixelFormat );
        SetLastError( ERROR_INVALID_PIXEL_FORMAT );
        ret = FALSE;
    }
    else
    {
        Drawable drawable = get_glxdrawable(physDev);
        Wine_GLContext *prev_ctx = NtCurrentTeb()->glContext;
        if (prev_ctx) prev_ctx->tid = 0;

        /* The describe lines below are for debugging purposes only */
        if (TRACE_ON(wgl)) {
            describeDrawable(physDev);
            describeContext(ctx);
        }

        TRACE(" make current for dis %p, drawable %p, ctx %p\n", gdi_display, (void*) drawable, ctx->ctx);
        ret = pglXMakeCurrent(gdi_display, drawable, ctx->ctx);
        NtCurrentTeb()->glContext = ctx;

        if(ret)
        {
            ctx->has_been_current = TRUE;
            ctx->tid = GetCurrentThreadId();
            ctx->hdc = hdc;
            ctx->read_hdc = hdc;
            ctx->drawables[0] = drawable;
            ctx->drawables[1] = drawable;
            ctx->refresh_drawables = FALSE;

            if (type == OBJ_MEMDC)
            {
                ctx->do_escape = TRUE;
                pglDrawBuffer(GL_FRONT_LEFT);
            }
        }
    }
    wine_tsx11_unlock();
    TRACE(" returning %s\n", (ret ? "True" : "False"));
    return ret;
}

/**
 * X11DRV_wglMakeContextCurrentARB
 *
 * For OpenGL32 wglMakeContextCurrentARB
 */
BOOL CDECL X11DRV_wglMakeContextCurrentARB(X11DRV_PDEVICE* pDrawDev, X11DRV_PDEVICE* pReadDev, HGLRC hglrc)
{
    BOOL ret;

    TRACE("(%p,%p,%p)\n", pDrawDev, pReadDev, hglrc);

    if (!has_opengl()) return 0;

    wine_tsx11_lock();
    if (hglrc == NULL)
    {
        Wine_GLContext *prev_ctx = NtCurrentTeb()->glContext;
        if (prev_ctx) prev_ctx->tid = 0;

        ret = pglXMakeCurrent(gdi_display, None, NULL);
        NtCurrentTeb()->glContext = NULL;
    }
    else
    {
        if (NULL == pglXMakeContextCurrent) {
            ret = FALSE;
        } else {
            Wine_GLContext *prev_ctx = NtCurrentTeb()->glContext;
            Wine_GLContext *ctx = (Wine_GLContext *) hglrc;
            Drawable d_draw = get_glxdrawable(pDrawDev);
            Drawable d_read = get_glxdrawable(pReadDev);

            if (prev_ctx) prev_ctx->tid = 0;

            ctx->has_been_current = TRUE;
            ctx->tid = GetCurrentThreadId();
            ctx->hdc = pDrawDev->hdc;
            ctx->read_hdc = pReadDev->hdc;
            ctx->drawables[0] = d_draw;
            ctx->drawables[1] = d_read;
            ctx->refresh_drawables = FALSE;
            ret = pglXMakeContextCurrent(gdi_display, d_draw, d_read, ctx->ctx);
            NtCurrentTeb()->glContext = ctx;
        }
    }
    wine_tsx11_unlock();

    TRACE(" returning %s\n", (ret ? "True" : "False"));
    return ret;
}

/**
 * X11DRV_wglShareLists
 *
 * For OpenGL32 wglShareLists.
 */
BOOL CDECL X11DRV_wglShareLists(HGLRC hglrc1, HGLRC hglrc2) {
    Wine_GLContext *org  = (Wine_GLContext *) hglrc1;
    Wine_GLContext *dest = (Wine_GLContext *) hglrc2;

    TRACE("(%p, %p)\n", org, dest);

    if (!has_opengl()) return FALSE;

    /* Sharing of display lists works differently in GLX and WGL. In case of GLX it is done
     * at context creation time but in case of WGL it is done using wglShareLists.
     * In the past we tried to emulate wglShareLists by delaying GLX context creation until
     * either a wglMakeCurrent or wglShareLists. This worked fine for most apps but it causes
     * issues for OpenGL 3 because there wglCreateContextAttribsARB can fail in a lot of cases,
     * so there delaying context creation doesn't work.
     *
     * The new approach is to create a GLX context in wglCreateContext / wglCreateContextAttribsARB
     * and when a program requests sharing we recreate the destination context if it hasn't been made
     * current or when it hasn't shared display lists before.
     */

    if((org->has_been_current && dest->has_been_current) || dest->has_been_current)
    {
        ERR("Could not share display lists, one of the contexts has been current already !\n");
        return FALSE;
    }
    else if(dest->sharing)
    {
        ERR("Could not share display lists because hglrc2 has already shared lists before\n");
        return FALSE;
    }
    else
    {
        if((GetObjectType(org->hdc) == OBJ_MEMDC) ^ (GetObjectType(dest->hdc) == OBJ_MEMDC))
        {
            WARN("Attempting to share a context between a direct and indirect rendering context, expect issues!\n");
        }

        wine_tsx11_lock();
        describeContext(org);
        describeContext(dest);

        /* Re-create the GLX context and share display lists */
        pglXDestroyContext(gdi_display, dest->ctx);
        dest->ctx = create_glxcontext(gdi_display, dest, org->ctx);
        wine_tsx11_unlock();
        TRACE(" re-created an OpenGL context (%p) for Wine context %p sharing lists with OpenGL ctx %p\n", dest->ctx, dest, org->ctx);

        org->sharing = TRUE;
        dest->sharing = TRUE;
        return TRUE;
    }
    return FALSE;
}

static BOOL internal_wglUseFontBitmaps(HDC hdc, DWORD first, DWORD count, DWORD listBase, DWORD (WINAPI *GetGlyphOutline_ptr)(HDC,UINT,UINT,LPGLYPHMETRICS,DWORD,LPVOID,const MAT2*))
{
     /* We are running using client-side rendering fonts... */
     GLYPHMETRICS gm;
     unsigned int glyph, size = 0;
     void *bitmap = NULL, *gl_bitmap = NULL;
     int org_alignment;

     wine_tsx11_lock();
     pglGetIntegerv(GL_UNPACK_ALIGNMENT, &org_alignment);
     pglPixelStorei(GL_UNPACK_ALIGNMENT, 4);
     wine_tsx11_unlock();

     for (glyph = first; glyph < first + count; glyph++) {
         static const MAT2 identity = { {0,1},{0,0},{0,0},{0,1} };
         unsigned int needed_size = GetGlyphOutline_ptr(hdc, glyph, GGO_BITMAP, &gm, 0, NULL, &identity);
         unsigned int height, width_int;

         TRACE("Glyph : %3d / List : %d\n", glyph, listBase);
         if (needed_size == GDI_ERROR) {
             TRACE("  - needed size : %d (GDI_ERROR)\n", needed_size);
             goto error;
         } else {
             TRACE("  - needed size : %d\n", needed_size);
         }

         if (needed_size > size) {
             size = needed_size;
             HeapFree(GetProcessHeap(), 0, bitmap);
             HeapFree(GetProcessHeap(), 0, gl_bitmap);
             bitmap = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
             gl_bitmap = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
         }
         if (GetGlyphOutline_ptr(hdc, glyph, GGO_BITMAP, &gm, size, bitmap, &identity) == GDI_ERROR)
             goto error;
         if (TRACE_ON(wgl)) {
             unsigned int height, width, bitmask;
             unsigned char *bitmap_ = bitmap;

             TRACE("  - bbox : %d x %d\n", gm.gmBlackBoxX, gm.gmBlackBoxY);
             TRACE("  - origin : (%d , %d)\n", gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y);
             TRACE("  - increment : %d - %d\n", gm.gmCellIncX, gm.gmCellIncY);
             if (needed_size != 0) {
                 TRACE("  - bitmap :\n");
                 for (height = 0; height < gm.gmBlackBoxY; height++) {
                     TRACE("      ");
                     for (width = 0, bitmask = 0x80; width < gm.gmBlackBoxX; width++, bitmask >>= 1) {
                         if (bitmask == 0) {
                             bitmap_ += 1;
                             bitmask = 0x80;
                         }
                         if (*bitmap_ & bitmask)
                             TRACE("*");
                         else
                             TRACE(" ");
                     }
                     bitmap_ += (4 - ((UINT_PTR)bitmap_ & 0x03));
                     TRACE("\n");
                 }
             }
         }

         /* In OpenGL, the bitmap is drawn from the bottom to the top... So we need to invert the
         * glyph for it to be drawn properly.
         */
         if (needed_size != 0) {
             width_int = (gm.gmBlackBoxX + 31) / 32;
             for (height = 0; height < gm.gmBlackBoxY; height++) {
                 unsigned int width;
                 for (width = 0; width < width_int; width++) {
                     ((int *) gl_bitmap)[(gm.gmBlackBoxY - height - 1) * width_int + width] =
                     ((int *) bitmap)[height * width_int + width];
                 }
             }
         }

         wine_tsx11_lock();
         pglNewList(listBase++, GL_COMPILE);
         if (needed_size != 0) {
             pglBitmap(gm.gmBlackBoxX, gm.gmBlackBoxY,
                     0 - gm.gmptGlyphOrigin.x, (int) gm.gmBlackBoxY - gm.gmptGlyphOrigin.y,
                     gm.gmCellIncX, gm.gmCellIncY,
                     gl_bitmap);
         } else {
             /* This is the case of 'empty' glyphs like the space character */
             pglBitmap(0, 0, 0, 0, gm.gmCellIncX, gm.gmCellIncY, NULL);
         }
         pglEndList();
         wine_tsx11_unlock();
     }

     wine_tsx11_lock();
     pglPixelStorei(GL_UNPACK_ALIGNMENT, org_alignment);
     wine_tsx11_unlock();

     HeapFree(GetProcessHeap(), 0, bitmap);
     HeapFree(GetProcessHeap(), 0, gl_bitmap);
     return TRUE;

  error:
     wine_tsx11_lock();
     pglPixelStorei(GL_UNPACK_ALIGNMENT, org_alignment);
     wine_tsx11_unlock();

     HeapFree(GetProcessHeap(), 0, bitmap);
     HeapFree(GetProcessHeap(), 0, gl_bitmap);
     return FALSE;
}

/**
 * X11DRV_wglUseFontBitmapsA
 *
 * For OpenGL32 wglUseFontBitmapsA.
 */
BOOL CDECL X11DRV_wglUseFontBitmapsA(X11DRV_PDEVICE *physDev, DWORD first, DWORD count, DWORD listBase)
{
     Font fid = physDev->font;

     TRACE("(%p, %d, %d, %d) using font %ld\n", physDev->hdc, first, count, listBase, fid);

     if (!has_opengl()) return FALSE;

     if (fid == 0) {
         return internal_wglUseFontBitmaps(physDev->hdc, first, count, listBase, GetGlyphOutlineA);
     }

     wine_tsx11_lock();
     /* I assume that the glyphs are at the same position for X and for Windows */
     pglXUseXFont(fid, first, count, listBase);
     wine_tsx11_unlock();
     return TRUE;
}

/**
 * X11DRV_wglUseFontBitmapsW
 *
 * For OpenGL32 wglUseFontBitmapsW.
 */
BOOL CDECL X11DRV_wglUseFontBitmapsW(X11DRV_PDEVICE *physDev, DWORD first, DWORD count, DWORD listBase)
{
     Font fid = physDev->font;

     TRACE("(%p, %d, %d, %d) using font %ld\n", physDev->hdc, first, count, listBase, fid);

     if (!has_opengl()) return FALSE;

     if (fid == 0) {
         return internal_wglUseFontBitmaps(physDev->hdc, first, count, listBase, GetGlyphOutlineW);
     }

     WARN("Using the glX API for the WCHAR variant - some characters may come out incorrectly !\n");

     wine_tsx11_lock();
     /* I assume that the glyphs are at the same position for X and for Windows */
     pglXUseXFont(fid, first, count, listBase);
     wine_tsx11_unlock();
     return TRUE;
}

/* WGL helper function which handles differences in glGetIntegerv from WGL and GLX */
static void WINAPI X11DRV_wglGetIntegerv(GLenum pname, GLint* params)
{
    wine_tsx11_lock();
    switch(pname)
    {
    case GL_DEPTH_BITS:
        {
            Wine_GLContext *ctx = NtCurrentTeb()->glContext;

            pglGetIntegerv(pname, params);
            /**
             * if we cannot find a Wine Context
             * we only have the default wine desktop context,
             * so if we have only a 24 depth say we have 32
             */
            if (!ctx && *params == 24) {
                *params = 32;
            }
            TRACE("returns GL_DEPTH_BITS as '%d'\n", *params);
            break;
        }
    case GL_ALPHA_BITS:
        {
            Wine_GLContext *ctx = NtCurrentTeb()->glContext;

            pglXGetFBConfigAttrib(gdi_display, ctx->fmt->fbconfig, GLX_ALPHA_SIZE, params);
            TRACE("returns GL_ALPHA_BITS as '%d'\n", *params);
            break;
        }
    default:
        pglGetIntegerv(pname, params);
        break;
    }
    wine_tsx11_unlock();
}

void flush_gl_drawable(X11DRV_PDEVICE *physDev)
{
    int w, h;

    if (!physDev->gl_copy)
        return;

    w = physDev->dc_rect.right - physDev->dc_rect.left;
    h = physDev->dc_rect.bottom - physDev->dc_rect.top;

    if(w > 0 && h > 0) {
        Drawable src = physDev->pixmap;
        if(!src) src = physDev->gl_drawable;

        /* The GL drawable may be lagged behind if we don't flush first, so
         * flush the display make sure we copy up-to-date data */
        wine_tsx11_lock();
        XFlush(gdi_display);
        XSetFunction(gdi_display, physDev->gc, GXcopy);
        XCopyArea(gdi_display, src, physDev->drawable, physDev->gc, 0, 0, w, h,
                  physDev->dc_rect.left, physDev->dc_rect.top);
        wine_tsx11_unlock();
    }
}


static void WINAPI X11DRV_wglFinish(void)
{
    Wine_GLContext *ctx = NtCurrentTeb()->glContext;
    enum x11drv_escape_codes code = X11DRV_FLUSH_GL_DRAWABLE;

    wine_tsx11_lock();
    sync_context(ctx);
    pglFinish();
    wine_tsx11_unlock();
    if (ctx) ExtEscape(ctx->hdc, X11DRV_ESCAPE, sizeof(code), (LPSTR)&code, 0, NULL );
}

static void WINAPI X11DRV_wglFlush(void)
{
    Wine_GLContext *ctx = NtCurrentTeb()->glContext;
    enum x11drv_escape_codes code = X11DRV_FLUSH_GL_DRAWABLE;

    wine_tsx11_lock();
    sync_context(ctx);
    pglFlush();
    wine_tsx11_unlock();
    if (ctx) ExtEscape(ctx->hdc, X11DRV_ESCAPE, sizeof(code), (LPSTR)&code, 0, NULL );
}

/**
 * X11DRV_wglCreateContextAttribsARB
 *
 * WGL_ARB_create_context: wglCreateContextAttribsARB
 */
HGLRC CDECL X11DRV_wglCreateContextAttribsARB(X11DRV_PDEVICE *physDev, HGLRC hShareContext, const int* attribList)
{
    Wine_GLContext *ret;
    WineGLPixelFormat *fmt;
    int hdcPF = physDev->current_pf;
    int fmt_count = 0;

    TRACE("(%p %p %p)\n", physDev, hShareContext, attribList);

    if (!has_opengl()) return 0;

    fmt = ConvertPixelFormatWGLtoGLX(gdi_display, hdcPF, TRUE /* Offscreen */, &fmt_count);
    /* wglCreateContextAttribsARB supports ALL pixel formats, so also offscreen ones.
     * If this fails something is very wrong on the system. */
    if(!fmt)
    {
        ERR("Cannot get FB Config for iPixelFormat %d, expect problems!\n", hdcPF);
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return NULL;
    }

    wine_tsx11_lock();
    ret = alloc_context();
    wine_tsx11_unlock();
    ret->hdc = physDev->hdc;
    ret->fmt = fmt;
    ret->vis = NULL; /* glXCreateContextAttribsARB requires a fbconfig instead of a visual */
    ret->gl3_context = TRUE;

    ret->numAttribs = 0;
    if(attribList)
    {
        int *pAttribList = (int*)attribList;
        int *pContextAttribList = &ret->attribList[0];
        /* attribList consists of pairs {token, value] terminated with 0 */
        while(pAttribList[0] != 0)
        {
            TRACE("%#x %#x\n", pAttribList[0], pAttribList[1]);
            switch(pAttribList[0])
            {
                case WGL_CONTEXT_MAJOR_VERSION_ARB:
                    pContextAttribList[0] = GLX_CONTEXT_MAJOR_VERSION_ARB;
                    pContextAttribList[1] = pAttribList[1];
                    break;
                case WGL_CONTEXT_MINOR_VERSION_ARB:
                    pContextAttribList[0] = GLX_CONTEXT_MINOR_VERSION_ARB;
                    pContextAttribList[1] = pAttribList[1];
                    break;
                case WGL_CONTEXT_LAYER_PLANE_ARB:
                    break;
                case WGL_CONTEXT_FLAGS_ARB:
                    pContextAttribList[0] = GLX_CONTEXT_FLAGS_ARB;
                    pContextAttribList[1] = pAttribList[1];
                    break;
                case WGL_CONTEXT_PROFILE_MASK_ARB:
                    pContextAttribList[0] = GLX_CONTEXT_PROFILE_MASK_ARB;
                    pContextAttribList[1] = pAttribList[1];
                    break;
                default:
                    ERR("Unhandled attribList pair: %#x %#x\n", pAttribList[0], pAttribList[1]);
            }

            ret->numAttribs++;
            pAttribList += 2;
            pContextAttribList += 2;
        }
    }

    wine_tsx11_lock();
    X11DRV_expect_error(gdi_display, GLXErrorHandler, NULL);
    ret->ctx = create_glxcontext(gdi_display, ret, NULL);

    XSync(gdi_display, False);
    if(X11DRV_check_error() || !ret->ctx)
    {
        /* In the future we should convert the GLX error to a win32 one here if needed */
        ERR("Context creation failed\n");
        free_context(ret);
        wine_tsx11_unlock();
        return NULL;
    }

    wine_tsx11_unlock();
    TRACE(" creating context %p\n", ret);
    return (HGLRC) ret;
}

/**
 * X11DRV_wglGetExtensionsStringARB
 *
 * WGL_ARB_extensions_string: wglGetExtensionsStringARB
 */
static const char * WINAPI X11DRV_wglGetExtensionsStringARB(HDC hdc) {
    TRACE("() returning \"%s\"\n", WineGLInfo.wglExtensions);
    return WineGLInfo.wglExtensions;
}

/**
 * X11DRV_wglCreatePbufferARB
 *
 * WGL_ARB_pbuffer: wglCreatePbufferARB
 */
static HPBUFFERARB WINAPI X11DRV_wglCreatePbufferARB(HDC hdc, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList)
{
    Wine_GLPBuffer* object = NULL;
    WineGLPixelFormat *fmt = NULL;
    int nCfgs = 0;
    int attribs[256];
    int nAttribs = 0;

    TRACE("(%p, %d, %d, %d, %p)\n", hdc, iPixelFormat, iWidth, iHeight, piAttribList);

    if (0 >= iPixelFormat) {
        ERR("(%p): unexpected iPixelFormat(%d) <= 0, returns NULL\n", hdc, iPixelFormat);
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return NULL; /* unexpected error */
    }

    /* Convert the WGL pixelformat to a GLX format, if it fails then the format is invalid */
    fmt = ConvertPixelFormatWGLtoGLX(gdi_display, iPixelFormat, TRUE /* Offscreen */, &nCfgs);
    if(!fmt) {
        ERR("(%p): unexpected iPixelFormat(%d) > nFormats(%d), returns NULL\n", hdc, iPixelFormat, nCfgs);
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        goto create_failed; /* unexpected error */
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Wine_GLPBuffer));
    if (NULL == object) {
        SetLastError(ERROR_NO_SYSTEM_RESOURCES);
        goto create_failed; /* unexpected error */
    }
    object->hdc = hdc;
    object->display = gdi_display;
    object->width = iWidth;
    object->height = iHeight;
    object->fmt = fmt;

    PUSH2(attribs, GLX_PBUFFER_WIDTH,  iWidth);
    PUSH2(attribs, GLX_PBUFFER_HEIGHT, iHeight); 
    while (piAttribList && 0 != *piAttribList) {
        int attr_v;
        switch (*piAttribList) {
            case WGL_PBUFFER_LARGEST_ARB: {
                ++piAttribList;
                attr_v = *piAttribList;
                TRACE("WGL_LARGEST_PBUFFER_ARB = %d\n", attr_v);
                PUSH2(attribs, GLX_LARGEST_PBUFFER, attr_v);
                break;
            }

            case WGL_TEXTURE_FORMAT_ARB: {
                ++piAttribList;
                attr_v = *piAttribList;
                TRACE("WGL_render_texture Attribute: WGL_TEXTURE_FORMAT_ARB as %x\n", attr_v);
                if (use_render_texture_ati) {
                    int type = 0;
                    switch (attr_v) {
                        case WGL_NO_TEXTURE_ARB: type = GLX_NO_TEXTURE_ATI; break ;
                        case WGL_TEXTURE_RGB_ARB: type = GLX_TEXTURE_RGB_ATI; break ;
                        case WGL_TEXTURE_RGBA_ARB: type = GLX_TEXTURE_RGBA_ATI; break ;
                        default:
                            SetLastError(ERROR_INVALID_DATA);
                            goto create_failed;
                    }
                    object->use_render_texture = 1;
                    PUSH2(attribs, GLX_TEXTURE_FORMAT_ATI, type);
                } else {
                    if (WGL_NO_TEXTURE_ARB == attr_v) {
                        object->use_render_texture = 0;
                    } else {
                        if (!use_render_texture_emulation) {
                            SetLastError(ERROR_INVALID_DATA);
                            goto create_failed;
                        }
                        switch (attr_v) {
                            case WGL_TEXTURE_RGB_ARB:
                                object->use_render_texture = GL_RGB;
                                object->texture_bpp = 3;
                                object->texture_format = GL_RGB;
                                object->texture_type = GL_UNSIGNED_BYTE;
                                break;
                            case WGL_TEXTURE_RGBA_ARB:
                                object->use_render_texture = GL_RGBA;
                                object->texture_bpp = 4;
                                object->texture_format = GL_RGBA;
                                object->texture_type = GL_UNSIGNED_BYTE;
                                break;

                            /* WGL_FLOAT_COMPONENTS_NV */
                            case WGL_TEXTURE_FLOAT_R_NV:
                                object->use_render_texture = GL_FLOAT_R_NV;
                                object->texture_bpp = 4;
                                object->texture_format = GL_RED;
                                object->texture_type = GL_FLOAT;
                                break;
                            case WGL_TEXTURE_FLOAT_RG_NV:
                                object->use_render_texture = GL_FLOAT_RG_NV;
                                object->texture_bpp = 8;
                                object->texture_format = GL_LUMINANCE_ALPHA;
                                object->texture_type = GL_FLOAT;
                                break;
                            case WGL_TEXTURE_FLOAT_RGB_NV:
                                object->use_render_texture = GL_FLOAT_RGB_NV;
                                object->texture_bpp = 12;
                                object->texture_format = GL_RGB;
                                object->texture_type = GL_FLOAT;
                                break;
                            case WGL_TEXTURE_FLOAT_RGBA_NV:
                                object->use_render_texture = GL_FLOAT_RGBA_NV;
                                object->texture_bpp = 16;
                                object->texture_format = GL_RGBA;
                                object->texture_type = GL_FLOAT;
                                break;
                            default:
                                ERR("Unknown texture format: %x\n", attr_v);
                                SetLastError(ERROR_INVALID_DATA);
                                goto create_failed;
                        }
                    }
                }
                break;
            }

            case WGL_TEXTURE_TARGET_ARB: {
                ++piAttribList;
                attr_v = *piAttribList;
                TRACE("WGL_render_texture Attribute: WGL_TEXTURE_TARGET_ARB as %x\n", attr_v);
                if (use_render_texture_ati) {
                    int type = 0;
                    switch (attr_v) {
                        case WGL_NO_TEXTURE_ARB: type = GLX_NO_TEXTURE_ATI; break ;
                        case WGL_TEXTURE_CUBE_MAP_ARB: type = GLX_TEXTURE_CUBE_MAP_ATI; break ;
                        case WGL_TEXTURE_1D_ARB: type = GLX_TEXTURE_1D_ATI; break ;
                        case WGL_TEXTURE_2D_ARB: type = GLX_TEXTURE_2D_ATI; break ;
                        default:
                            SetLastError(ERROR_INVALID_DATA);
                            goto create_failed;
                    }
                    PUSH2(attribs, GLX_TEXTURE_TARGET_ATI, type);
                } else {
                    if (WGL_NO_TEXTURE_ARB == attr_v) {
                        object->texture_target = 0;
                    } else {
                        if (!use_render_texture_emulation) {
                            SetLastError(ERROR_INVALID_DATA);
                            goto create_failed;
                        }
                        switch (attr_v) {
                            case WGL_TEXTURE_CUBE_MAP_ARB: {
                                if (iWidth != iHeight) {
                                    SetLastError(ERROR_INVALID_DATA);
                                    goto create_failed;
                                }
                                object->texture_target = GL_TEXTURE_CUBE_MAP;
                                object->texture_bind_target = GL_TEXTURE_BINDING_CUBE_MAP;
                               break;
                            }
                            case WGL_TEXTURE_1D_ARB: {
                                if (1 != iHeight) {
                                    SetLastError(ERROR_INVALID_DATA);
                                    goto create_failed;
                                }
                                object->texture_target = GL_TEXTURE_1D;
                                object->texture_bind_target = GL_TEXTURE_BINDING_1D;
                                break;
                            }
                            case WGL_TEXTURE_2D_ARB: {
                                object->texture_target = GL_TEXTURE_2D;
                                object->texture_bind_target = GL_TEXTURE_BINDING_2D;
                                break;
                            }
                            case WGL_TEXTURE_RECTANGLE_NV: {
                                object->texture_target = GL_TEXTURE_RECTANGLE_NV;
                                object->texture_bind_target = GL_TEXTURE_BINDING_RECTANGLE_NV;
                                break;
                            }
                            default:
                                ERR("Unknown texture target: %x\n", attr_v);
                                SetLastError(ERROR_INVALID_DATA);
                                goto create_failed;
                        }
                    }
                }
                break;
            }

            case WGL_MIPMAP_TEXTURE_ARB: {
                ++piAttribList;
                attr_v = *piAttribList;
                TRACE("WGL_render_texture Attribute: WGL_MIPMAP_TEXTURE_ARB as %x\n", attr_v);
                if (use_render_texture_ati) {
                    PUSH2(attribs, GLX_MIPMAP_TEXTURE_ATI, attr_v);
                } else {
                    if (!use_render_texture_emulation) {
                        SetLastError(ERROR_INVALID_DATA);
                        goto create_failed;
                    }
                }
                break;
            }
        }
        ++piAttribList;
    }

    PUSH1(attribs, None);
    wine_tsx11_lock();
    object->drawable = pglXCreatePbuffer(gdi_display, fmt->fbconfig, attribs);
    wine_tsx11_unlock();
    TRACE("new Pbuffer drawable as %p\n", (void*) object->drawable);
    if (!object->drawable) {
        SetLastError(ERROR_NO_SYSTEM_RESOURCES);
        goto create_failed; /* unexpected error */
    }
    TRACE("->(%p)\n", object);
    return object;

create_failed:
    HeapFree(GetProcessHeap(), 0, object);
    TRACE("->(FAILED)\n");
    return NULL;
}

/**
 * X11DRV_wglDestroyPbufferARB
 *
 * WGL_ARB_pbuffer: wglDestroyPbufferARB
 */
static GLboolean WINAPI X11DRV_wglDestroyPbufferARB(HPBUFFERARB hPbuffer)
{
    Wine_GLPBuffer* object = hPbuffer;
    TRACE("(%p)\n", hPbuffer);
    if (NULL == object) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    wine_tsx11_lock();
    pglXDestroyPbuffer(object->display, object->drawable);
    wine_tsx11_unlock();
    HeapFree(GetProcessHeap(), 0, object);
    return GL_TRUE;
}

/**
 * X11DRV_wglGetPbufferDCARB
 *
 * WGL_ARB_pbuffer: wglGetPbufferDCARB
 * The function wglGetPbufferDCARB returns a device context for a pbuffer.
 * Gdi32 implements the part of this function which creates a device context.
 * This part associates the physDev with the X drawable of the pbuffer.
 */
HDC CDECL X11DRV_wglGetPbufferDCARB(X11DRV_PDEVICE *physDev, HPBUFFERARB hPbuffer)
{
    Wine_GLPBuffer* object = hPbuffer;
    if (NULL == object) {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    /* The function wglGetPbufferDCARB returns a DC to which the pbuffer can be connected.
     * All formats in our pixelformat list are compatible with each other and the main drawable. */
    physDev->current_pf = object->fmt->iPixelFormat;
    physDev->drawable = object->drawable;
    SetRect( &physDev->drawable_rect, 0, 0, object->width, object->height );
    physDev->dc_rect = physDev->drawable_rect;

    TRACE("(%p)->(%p)\n", hPbuffer, physDev->hdc);
    return physDev->hdc;
}

/**
 * X11DRV_wglQueryPbufferARB
 *
 * WGL_ARB_pbuffer: wglQueryPbufferARB
 */
static GLboolean WINAPI X11DRV_wglQueryPbufferARB(HPBUFFERARB hPbuffer, int iAttribute, int *piValue)
{
    Wine_GLPBuffer* object = hPbuffer;
    TRACE("(%p, 0x%x, %p)\n", hPbuffer, iAttribute, piValue);
    if (NULL == object) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    switch (iAttribute) {
        case WGL_PBUFFER_WIDTH_ARB:
            wine_tsx11_lock();
            pglXQueryDrawable(object->display, object->drawable, GLX_WIDTH, (unsigned int*) piValue);
            wine_tsx11_unlock();
            break;
        case WGL_PBUFFER_HEIGHT_ARB:
            wine_tsx11_lock();
            pglXQueryDrawable(object->display, object->drawable, GLX_HEIGHT, (unsigned int*) piValue);
            wine_tsx11_unlock();
            break;

        case WGL_PBUFFER_LOST_ARB:
            /* GLX Pbuffers cannot be lost by default. We can support this by
             * setting GLX_PRESERVED_CONTENTS to False and using glXSelectEvent
             * to receive pixel buffer clobber events, however that may or may
             * not give any benefit */
            *piValue = GL_FALSE;
            break;

        case WGL_TEXTURE_FORMAT_ARB:
            if (use_render_texture_ati) {
                unsigned int tmp;
                int type = WGL_NO_TEXTURE_ARB;
                wine_tsx11_lock();
                pglXQueryDrawable(object->display, object->drawable, GLX_TEXTURE_FORMAT_ATI, &tmp);
                wine_tsx11_unlock();
                switch (tmp) {
                    case GLX_NO_TEXTURE_ATI: type = WGL_NO_TEXTURE_ARB; break ;
                    case GLX_TEXTURE_RGB_ATI: type = WGL_TEXTURE_RGB_ARB; break ;
                    case GLX_TEXTURE_RGBA_ATI: type = WGL_TEXTURE_RGBA_ARB; break ;
                }
                *piValue = type;
            } else {
                if (!object->use_render_texture) {
                    *piValue = WGL_NO_TEXTURE_ARB;
                } else {
                    if (!use_render_texture_emulation) {
                        SetLastError(ERROR_INVALID_HANDLE);
                        return GL_FALSE;
                    }
                    switch(object->use_render_texture) {
                        case GL_RGB:
                            *piValue = WGL_TEXTURE_RGB_ARB;
                            break;
                        case GL_RGBA:
                            *piValue = WGL_TEXTURE_RGBA_ARB;
                            break;
                        /* WGL_FLOAT_COMPONENTS_NV */
                        case GL_FLOAT_R_NV:
                            *piValue = WGL_TEXTURE_FLOAT_R_NV;
                            break;
                        case GL_FLOAT_RG_NV:
                            *piValue = WGL_TEXTURE_FLOAT_RG_NV;
                            break;
                        case GL_FLOAT_RGB_NV:
                            *piValue = WGL_TEXTURE_FLOAT_RGB_NV;
                            break;
                        case GL_FLOAT_RGBA_NV:
                            *piValue = WGL_TEXTURE_FLOAT_RGBA_NV;
                            break;
                        default:
                            ERR("Unknown texture format: %x\n", object->use_render_texture);
                    }
                }
            }
            break;

        case WGL_TEXTURE_TARGET_ARB:
            if (use_render_texture_ati) {
                unsigned int tmp;
                int type = WGL_NO_TEXTURE_ARB;
                wine_tsx11_lock();
                pglXQueryDrawable(object->display, object->drawable, GLX_TEXTURE_TARGET_ATI, &tmp);
                wine_tsx11_unlock();
                switch (tmp) {
                    case GLX_NO_TEXTURE_ATI: type = WGL_NO_TEXTURE_ARB; break ;
                    case GLX_TEXTURE_CUBE_MAP_ATI: type = WGL_TEXTURE_CUBE_MAP_ARB; break ;
                    case GLX_TEXTURE_1D_ATI: type = WGL_TEXTURE_1D_ARB; break ;
                    case GLX_TEXTURE_2D_ATI: type = WGL_TEXTURE_2D_ARB; break ;
                }
                *piValue = type;
            } else {
            if (!object->texture_target) {
                *piValue = WGL_NO_TEXTURE_ARB;
            } else {
                if (!use_render_texture_emulation) {
                    SetLastError(ERROR_INVALID_DATA);      
                    return GL_FALSE;
                }
                switch (object->texture_target) {
                    case GL_TEXTURE_1D:       *piValue = WGL_TEXTURE_1D_ARB; break;
                    case GL_TEXTURE_2D:       *piValue = WGL_TEXTURE_2D_ARB; break;
                    case GL_TEXTURE_CUBE_MAP: *piValue = WGL_TEXTURE_CUBE_MAP_ARB; break;
                    case GL_TEXTURE_RECTANGLE_NV: *piValue = WGL_TEXTURE_RECTANGLE_NV; break;
                }
            }
        }
        break;

    case WGL_MIPMAP_TEXTURE_ARB:
        if (use_render_texture_ati) {
            wine_tsx11_lock();
            pglXQueryDrawable(object->display, object->drawable, GLX_MIPMAP_TEXTURE_ATI, (unsigned int*) piValue);
            wine_tsx11_unlock();
        } else {
            *piValue = GL_FALSE; /** don't support that */
            FIXME("unsupported WGL_ARB_render_texture attribute query for 0x%x\n", iAttribute);
        }
        break;

    default:
        FIXME("unexpected attribute %x\n", iAttribute);
        break;
    }

    return GL_TRUE;
}

/**
 * X11DRV_wglReleasePbufferDCARB
 *
 * WGL_ARB_pbuffer: wglReleasePbufferDCARB
 */
static int WINAPI X11DRV_wglReleasePbufferDCARB(HPBUFFERARB hPbuffer, HDC hdc)
{
    TRACE("(%p, %p)\n", hPbuffer, hdc);
    return DeleteDC(hdc);
}

/**
 * X11DRV_wglSetPbufferAttribARB
 *
 * WGL_ARB_pbuffer: wglSetPbufferAttribARB
 */
static GLboolean WINAPI X11DRV_wglSetPbufferAttribARB(HPBUFFERARB hPbuffer, const int *piAttribList)
{
    Wine_GLPBuffer* object = hPbuffer;
    GLboolean ret = GL_FALSE;

    WARN("(%p, %p): alpha-testing, report any problem\n", hPbuffer, piAttribList);
    if (NULL == object) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    if (!object->use_render_texture) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    if (!use_render_texture_ati && 1 == use_render_texture_emulation) {
        return GL_TRUE;
    }
    if (NULL != pglXDrawableAttribATI) {
        if (use_render_texture_ati) {
            FIXME("Need conversion for GLX_ATI_render_texture\n");
        }
        wine_tsx11_lock();
        ret = pglXDrawableAttribATI(object->display, object->drawable, piAttribList);
        wine_tsx11_unlock();
    }
    return ret;
}

/**
 * X11DRV_wglChoosePixelFormatARB
 *
 * WGL_ARB_pixel_format: wglChoosePixelFormatARB
 */
static GLboolean WINAPI X11DRV_wglChoosePixelFormatARB(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats)
{
    int gl_test = 0;
    int attribs[256];
    int nAttribs = 0;
    GLXFBConfig* cfgs = NULL;
    int nCfgs = 0;
    int it;
    int fmt_id;
    WineGLPixelFormat *fmt;
    UINT pfmt_it = 0;
    int run;
    int i;
    DWORD dwFlags = 0;

    TRACE("(%p, %p, %p, %d, %p, %p): hackish\n", hdc, piAttribIList, pfAttribFList, nMaxFormats, piFormats, nNumFormats);
    if (NULL != pfAttribFList) {
        FIXME("unused pfAttribFList\n");
    }

    nAttribs = ConvertAttribWGLtoGLX(piAttribIList, attribs, NULL);
    if (-1 == nAttribs) {
        WARN("Cannot convert WGL to GLX attributes\n");
        return GL_FALSE;
    }
    PUSH1(attribs, None);

    /* There is no 1:1 mapping between GLX and WGL formats because we duplicate some GLX formats for bitmap rendering (see get_formats).
     * Flags like PFD_SUPPORT_GDI, PFD_DRAW_TO_BITMAP and others are a property of the WineGLPixelFormat. We don't query these attributes
     * using glXChooseFBConfig but we filter the result of glXChooseFBConfig later on by passing a dwFlags to 'ConvertPixelFormatGLXtoWGL'. */
    for(i=0; piAttribIList[i] != 0; i+=2)
    {
        switch(piAttribIList[i])
        {
            case WGL_DRAW_TO_BITMAP_ARB:
                if(piAttribIList[i+1])
                    dwFlags |= PFD_DRAW_TO_BITMAP;
                break;
            case WGL_ACCELERATION_ARB:
                switch(piAttribIList[i+1])
                {
                    case WGL_NO_ACCELERATION_ARB:
                        dwFlags |= PFD_GENERIC_FORMAT;
                        break;
                    case WGL_GENERIC_ACCELERATION_ARB:
                        dwFlags |= PFD_GENERIC_ACCELERATED;
                        break;
                    case WGL_FULL_ACCELERATION_ARB:
                        /* Nothing to do */
                        break;
                }
                break;
            case WGL_SUPPORT_GDI_ARB:
                if(piAttribIList[i+1])
                    dwFlags |= PFD_SUPPORT_GDI;
                break;
        }
    }

    /* Search for FB configurations matching the requirements in attribs */
    wine_tsx11_lock();
    cfgs = pglXChooseFBConfig(gdi_display, DefaultScreen(gdi_display), attribs, &nCfgs);
    if (NULL == cfgs) {
        wine_tsx11_unlock();
        WARN("Compatible Pixel Format not found\n");
        return GL_FALSE;
    }

    /* Loop through all matching formats and check if they are suitable.
    * Note that this function should at max return nMaxFormats different formats */
    for(run=0; run < 2; run++)
    {
        for (it = 0; it < nCfgs; ++it) {
            gl_test = pglXGetFBConfigAttrib(gdi_display, cfgs[it], GLX_FBCONFIG_ID, &fmt_id);
            if (gl_test) {
                ERR("Failed to retrieve FBCONFIG_ID from GLXFBConfig, expect problems.\n");
                continue;
            }

            /* Search for the format in our list of compatible formats */
            fmt = ConvertPixelFormatGLXtoWGL(gdi_display, fmt_id, dwFlags);
            if(!fmt)
                continue;

            /* During the first run we only want onscreen formats and during the second only offscreen 'XOR' */
            if( ((run == 0) && fmt->offscreenOnly) || ((run == 1) && !fmt->offscreenOnly) )
                continue;

            if(pfmt_it < nMaxFormats) {
                piFormats[pfmt_it] = fmt->iPixelFormat;
                TRACE("at %d/%d found FBCONFIG_ID 0x%x (%d)\n", it + 1, nCfgs, fmt_id, piFormats[pfmt_it]);
            }
            pfmt_it++;
        }
    }

    *nNumFormats = pfmt_it;
    /** free list */
    XFree(cfgs);
    wine_tsx11_unlock();
    return GL_TRUE;
}

/**
 * X11DRV_wglGetPixelFormatAttribivARB
 *
 * WGL_ARB_pixel_format: wglGetPixelFormatAttribivARB
 */
static GLboolean WINAPI X11DRV_wglGetPixelFormatAttribivARB(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues)
{
    UINT i;
    WineGLPixelFormat *fmt = NULL;
    int hTest;
    int tmp;
    int curGLXAttr = 0;
    int nWGLFormats = 0;

    TRACE("(%p, %d, %d, %d, %p, %p)\n", hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, piValues);

    if (0 < iLayerPlane) {
        FIXME("unsupported iLayerPlane(%d) > 0, returns FALSE\n", iLayerPlane);
        return GL_FALSE;
    }

    /* Convert the WGL pixelformat to a GLX one, if this fails then most likely the iPixelFormat isn't supported.
    * We don't have to fail yet as a program can specify an invalid iPixelFormat (lets say 0) if it wants to query
    * the number of supported WGL formats. Whether the iPixelFormat is valid is handled in the for-loop below. */
    fmt = ConvertPixelFormatWGLtoGLX(gdi_display, iPixelFormat, TRUE /* Offscreen */, &nWGLFormats);
    if(!fmt) {
        WARN("Unable to convert iPixelFormat %d to a GLX one!\n", iPixelFormat);
    }

    wine_tsx11_lock();
    for (i = 0; i < nAttributes; ++i) {
        const int curWGLAttr = piAttributes[i];
        TRACE("pAttr[%d] = %x\n", i, curWGLAttr);

        switch (curWGLAttr) {
            case WGL_NUMBER_PIXEL_FORMATS_ARB:
                piValues[i] = nWGLFormats; 
                continue;

            case WGL_SUPPORT_OPENGL_ARB:
                piValues[i] = GL_TRUE; 
                continue;

            case WGL_ACCELERATION_ARB:
                curGLXAttr = GLX_CONFIG_CAVEAT;
                if (!fmt) goto pix_error;
                if(fmt->dwFlags & PFD_GENERIC_FORMAT)
                    piValues[i] = WGL_NO_ACCELERATION_ARB;
                else if(fmt->dwFlags & PFD_GENERIC_ACCELERATED)
                    piValues[i] = WGL_GENERIC_ACCELERATION_ARB;
                else
                    piValues[i] = WGL_FULL_ACCELERATION_ARB;
                continue;

            case WGL_TRANSPARENT_ARB:
                curGLXAttr = GLX_TRANSPARENT_TYPE;
                if (!fmt) goto pix_error;
                hTest = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, curGLXAttr, &tmp);
                if (hTest) goto get_error;
                    piValues[i] = GL_FALSE;
                if (GLX_NONE != tmp) piValues[i] = GL_TRUE;
                    continue;

            case WGL_PIXEL_TYPE_ARB:
                curGLXAttr = GLX_RENDER_TYPE;
                if (!fmt) goto pix_error;
                hTest = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, curGLXAttr, &tmp);
                if (hTest) goto get_error;
                TRACE("WGL_PIXEL_TYPE_ARB: GLX_RENDER_TYPE = 0x%x\n", tmp);
                if      (tmp & GLX_RGBA_BIT)           { piValues[i] = WGL_TYPE_RGBA_ARB; }
                else if (tmp & GLX_COLOR_INDEX_BIT)    { piValues[i] = WGL_TYPE_COLORINDEX_ARB; }
                else if (tmp & GLX_RGBA_FLOAT_BIT)     { piValues[i] = WGL_TYPE_RGBA_FLOAT_ATI; }
                else if (tmp & GLX_RGBA_FLOAT_ATI_BIT) { piValues[i] = WGL_TYPE_RGBA_FLOAT_ATI; }
                else if (tmp & GLX_RGBA_UNSIGNED_FLOAT_BIT_EXT) { piValues[i] = WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT; }
                else {
                    ERR("unexpected RenderType(%x)\n", tmp);
                    piValues[i] = WGL_TYPE_RGBA_ARB;
                }
                continue;

            case WGL_COLOR_BITS_ARB:
                curGLXAttr = GLX_BUFFER_SIZE;
                break;

            case WGL_BIND_TO_TEXTURE_RGB_ARB:
                if (use_render_texture_ati) {
                    curGLXAttr = GLX_BIND_TO_TEXTURE_RGB_ATI;
                    break;
                }
            case WGL_BIND_TO_TEXTURE_RGBA_ARB:
                if (use_render_texture_ati) {
                    curGLXAttr = GLX_BIND_TO_TEXTURE_RGBA_ATI;
                    break;
                }
                if (!use_render_texture_emulation) {
                    piValues[i] = GL_FALSE;
                    continue;	
                }
                curGLXAttr = GLX_RENDER_TYPE;
                if (!fmt) goto pix_error;
                hTest = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, curGLXAttr, &tmp);
                if (hTest) goto get_error;
                if (GLX_COLOR_INDEX_BIT == tmp) {
                    piValues[i] = GL_FALSE;  
                    continue;
                }
                hTest = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_DRAWABLE_TYPE, &tmp);
                if (hTest) goto get_error;
                piValues[i] = (tmp & GLX_PBUFFER_BIT) ? GL_TRUE : GL_FALSE;
                continue;

            case WGL_BLUE_BITS_ARB:
                curGLXAttr = GLX_BLUE_SIZE;
                break;
            case WGL_RED_BITS_ARB:
                curGLXAttr = GLX_RED_SIZE;
                break;
            case WGL_GREEN_BITS_ARB:
                curGLXAttr = GLX_GREEN_SIZE;
                break;
            case WGL_ALPHA_BITS_ARB:
                curGLXAttr = GLX_ALPHA_SIZE;
                break;
            case WGL_DEPTH_BITS_ARB:
                curGLXAttr = GLX_DEPTH_SIZE;
                break;
            case WGL_STENCIL_BITS_ARB:
                curGLXAttr = GLX_STENCIL_SIZE;
                break;
            case WGL_DOUBLE_BUFFER_ARB:
                curGLXAttr = GLX_DOUBLEBUFFER;
                break;
            case WGL_STEREO_ARB:
                curGLXAttr = GLX_STEREO;
                break;
            case WGL_AUX_BUFFERS_ARB:
                curGLXAttr = GLX_AUX_BUFFERS;
                break;

            case WGL_SUPPORT_GDI_ARB:
                if (!fmt) goto pix_error;
                piValues[i] = (fmt->dwFlags & PFD_SUPPORT_GDI) ? TRUE : FALSE;
                continue;

            case WGL_DRAW_TO_BITMAP_ARB:
                if (!fmt) goto pix_error;
                piValues[i] = (fmt->dwFlags & PFD_DRAW_TO_BITMAP) ? TRUE : FALSE;
                continue;

            case WGL_DRAW_TO_WINDOW_ARB:
            case WGL_DRAW_TO_PBUFFER_ARB:
                if (!fmt) goto pix_error;
                hTest = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_DRAWABLE_TYPE, &tmp);
                if (hTest) goto get_error;
                if((curWGLAttr == WGL_DRAW_TO_WINDOW_ARB && (tmp&GLX_WINDOW_BIT)) ||
                   (curWGLAttr == WGL_DRAW_TO_PBUFFER_ARB && (tmp&GLX_PBUFFER_BIT)))
                    piValues[i] = GL_TRUE;
                else
                    piValues[i] = GL_FALSE;
                continue;

            case WGL_SWAP_METHOD_ARB:
                /* For now return SWAP_EXCHANGE_ARB which is the best type of buffer switch available.
                 * Later on we can also use GLX_OML_swap_method on drivers which support this. At this
                 * point only ATI offers this.
                 */
                piValues[i] = WGL_SWAP_EXCHANGE_ARB;
                break;

            case WGL_PBUFFER_LARGEST_ARB:
                curGLXAttr = GLX_LARGEST_PBUFFER;
                break;

            case WGL_SAMPLE_BUFFERS_ARB:
                curGLXAttr = GLX_SAMPLE_BUFFERS_ARB;
                break;

            case WGL_SAMPLES_ARB:
                curGLXAttr = GLX_SAMPLES_ARB;
                break;

            case WGL_FLOAT_COMPONENTS_NV:
                curGLXAttr = GLX_FLOAT_COMPONENTS_NV;
                break;

            case WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT:
                curGLXAttr = GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT;
                break;

            case WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT:
                curGLXAttr = GLX_RGBA_UNSIGNED_FLOAT_TYPE_EXT;
                break;

            case WGL_ACCUM_RED_BITS_ARB:
                curGLXAttr = GLX_ACCUM_RED_SIZE;
                break;
            case WGL_ACCUM_GREEN_BITS_ARB:
                curGLXAttr = GLX_ACCUM_GREEN_SIZE;
                break;
            case WGL_ACCUM_BLUE_BITS_ARB:
                curGLXAttr = GLX_ACCUM_BLUE_SIZE;
                break;
            case WGL_ACCUM_ALPHA_BITS_ARB:
                curGLXAttr = GLX_ACCUM_ALPHA_SIZE;
                break;
            case WGL_ACCUM_BITS_ARB:
                if (!fmt) goto pix_error;
                hTest = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_ACCUM_RED_SIZE, &tmp);
                if (hTest) goto get_error;
                piValues[i] = tmp;
                hTest = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_ACCUM_GREEN_SIZE, &tmp);
                if (hTest) goto get_error;
                piValues[i] += tmp;
                hTest = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_ACCUM_BLUE_SIZE, &tmp);
                if (hTest) goto get_error;
                piValues[i] += tmp;
                hTest = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, GLX_ACCUM_ALPHA_SIZE, &tmp);
                if (hTest) goto get_error;
                piValues[i] += tmp;
                continue;

            default:
                FIXME("unsupported %x WGL Attribute\n", curWGLAttr);
        }

        /* Retrieve a GLX FBConfigAttrib when the attribute to query is valid and
         * iPixelFormat != 0. When iPixelFormat is 0 the only value which makes
         * sense to query is WGL_NUMBER_PIXEL_FORMATS_ARB.
         *
         * TODO: properly test the behavior of wglGetPixelFormatAttrib*v on Windows
         *       and check which options can work using iPixelFormat=0 and which not.
         *       A problem would be that this function is an extension. This would
         *       mean that the behavior could differ between different vendors (ATI, Nvidia, ..).
         */
        if (0 != curGLXAttr && iPixelFormat != 0) {
            if (!fmt) goto pix_error;
            hTest = pglXGetFBConfigAttrib(gdi_display, fmt->fbconfig, curGLXAttr, piValues + i);
            if (hTest) goto get_error;
            curGLXAttr = 0;
        } else { 
            piValues[i] = GL_FALSE; 
        }
    }
    wine_tsx11_unlock();
    return GL_TRUE;

get_error:
    wine_tsx11_unlock();
    ERR("(%p): unexpected failure on GetFBConfigAttrib(%x) returns FALSE\n", hdc, curGLXAttr);
    return GL_FALSE;

pix_error:
    wine_tsx11_unlock();
    ERR("(%p): unexpected iPixelFormat(%d) vs nFormats(%d), returns FALSE\n", hdc, iPixelFormat, nWGLFormats);
    return GL_FALSE;
}

/**
 * X11DRV_wglGetPixelFormatAttribfvARB
 *
 * WGL_ARB_pixel_format: wglGetPixelFormatAttribfvARB
 */
static GLboolean WINAPI X11DRV_wglGetPixelFormatAttribfvARB(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues)
{
    int *attr;
    int ret;
    UINT i;

    TRACE("(%p, %d, %d, %d, %p, %p)\n", hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, pfValues);

    /* Allocate a temporary array to store integer values */
    attr = HeapAlloc(GetProcessHeap(), 0, nAttributes * sizeof(int));
    if (!attr) {
        ERR("couldn't allocate %d array\n", nAttributes);
        return GL_FALSE;
    }

    /* Piggy-back on wglGetPixelFormatAttribivARB */
    ret = X11DRV_wglGetPixelFormatAttribivARB(hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, attr);
    if (ret) {
        /* Convert integer values to float. Should also check for attributes
           that can give decimal values here */
        for (i=0; i<nAttributes;i++) {
            pfValues[i] = attr[i];
        }
    }

    HeapFree(GetProcessHeap(), 0, attr);
    return ret;
}

/**
 * X11DRV_wglBindTexImageARB
 *
 * WGL_ARB_render_texture: wglBindTexImageARB
 */
static GLboolean WINAPI X11DRV_wglBindTexImageARB(HPBUFFERARB hPbuffer, int iBuffer)
{
    Wine_GLPBuffer* object = hPbuffer;
    GLboolean ret = GL_FALSE;

    TRACE("(%p, %d)\n", hPbuffer, iBuffer);
    if (NULL == object) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    if (!object->use_render_texture) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }

    if (!use_render_texture_ati && 1 == use_render_texture_emulation) {
        static int init = 0;
        int prev_binded_texture = 0;
        GLXContext prev_context;
        Drawable prev_drawable;
        GLXContext tmp_context;

        wine_tsx11_lock();
        prev_context = pglXGetCurrentContext();
        prev_drawable = pglXGetCurrentDrawable();

        /* Our render_texture emulation is basic and lacks some features (1D/Cube support).
           This is mostly due to lack of demos/games using them. Further the use of glReadPixels
           isn't ideal performance wise but I wasn't able to get other ways working.
        */
        if(!init) {
            init = 1; /* Only show the FIXME once for performance reasons */
            FIXME("partial stub!\n");
        }

        TRACE("drawable=%p, context=%p\n", (void*)object->drawable, prev_context);
        tmp_context = pglXCreateNewContext(gdi_display, object->fmt->fbconfig, object->fmt->render_type, prev_context, True);

        pglGetIntegerv(object->texture_bind_target, &prev_binded_texture);

        /* Switch to our pbuffer */
        pglXMakeCurrent(gdi_display, object->drawable, tmp_context);

        /* Make sure that the prev_binded_texture is set as the current texture state isn't shared between contexts.
         * After that upload the pbuffer texture data. */
        pglBindTexture(object->texture_target, prev_binded_texture);
        pglCopyTexImage2D(object->texture_target, 0, object->use_render_texture, 0, 0, object->width, object->height, 0);

        /* Switch back to the original drawable and upload the pbuffer-texture */
        pglXMakeCurrent(object->display, prev_drawable, prev_context);
        pglXDestroyContext(gdi_display, tmp_context);
        wine_tsx11_unlock();
        return GL_TRUE;
    }

    if (NULL != pglXBindTexImageATI) {
        int buffer;

        switch(iBuffer)
        {
            case WGL_FRONT_LEFT_ARB:
                buffer = GLX_FRONT_LEFT_ATI;
                break;
            case WGL_FRONT_RIGHT_ARB:
                buffer = GLX_FRONT_RIGHT_ATI;
                break;
            case WGL_BACK_LEFT_ARB:
                buffer = GLX_BACK_LEFT_ATI;
                break;
            case WGL_BACK_RIGHT_ARB:
                buffer = GLX_BACK_RIGHT_ATI;
                break;
            default:
                ERR("Unknown iBuffer=%#x\n", iBuffer);
                return FALSE;
        }

        /* In the sample 'ogl_offscreen_rendering_3' from codesampler.net I get garbage on the screen.
         * I'm not sure if that's a bug in the ATI extension or in the program. I think that the program
         * expected a single buffering format since it didn't ask for double buffering. A buffer swap
         * fixed the program. I don't know what the correct behavior is. On the other hand that demo
         * works fine using our pbuffer emulation path.
         */
        wine_tsx11_lock();
        ret = pglXBindTexImageATI(object->display, object->drawable, buffer);
        wine_tsx11_unlock();
    }
    return ret;
}

/**
 * X11DRV_wglReleaseTexImageARB
 *
 * WGL_ARB_render_texture: wglReleaseTexImageARB
 */
static GLboolean WINAPI X11DRV_wglReleaseTexImageARB(HPBUFFERARB hPbuffer, int iBuffer)
{
    Wine_GLPBuffer* object = hPbuffer;
    GLboolean ret = GL_FALSE;

    TRACE("(%p, %d)\n", hPbuffer, iBuffer);
    if (NULL == object) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    if (!object->use_render_texture) {
        SetLastError(ERROR_INVALID_HANDLE);
        return GL_FALSE;
    }
    if (!use_render_texture_ati && 1 == use_render_texture_emulation) {
        return GL_TRUE;
    }
    if (NULL != pglXReleaseTexImageATI) {
        int buffer;

        switch(iBuffer)
        {
            case WGL_FRONT_LEFT_ARB:
                buffer = GLX_FRONT_LEFT_ATI;
                break;
            case WGL_FRONT_RIGHT_ARB:
                buffer = GLX_FRONT_RIGHT_ATI;
                break;
            case WGL_BACK_LEFT_ARB:
                buffer = GLX_BACK_LEFT_ATI;
                break;
            case WGL_BACK_RIGHT_ARB:
                buffer = GLX_BACK_RIGHT_ATI;
                break;
            default:
                ERR("Unknown iBuffer=%#x\n", iBuffer);
                return FALSE;
        }
        wine_tsx11_lock();
        ret = pglXReleaseTexImageATI(object->display, object->drawable, buffer);
        wine_tsx11_unlock();
    }
    return ret;
}

/**
 * X11DRV_wglGetExtensionsStringEXT
 *
 * WGL_EXT_extensions_string: wglGetExtensionsStringEXT
 */
static const char * WINAPI X11DRV_wglGetExtensionsStringEXT(void) {
    TRACE("() returning \"%s\"\n", WineGLInfo.wglExtensions);
    return WineGLInfo.wglExtensions;
}

/**
 * X11DRV_wglGetSwapIntervalEXT
 *
 * WGL_EXT_swap_control: wglGetSwapIntervalEXT
 */
static int WINAPI X11DRV_wglGetSwapIntervalEXT(VOID) {
    FIXME("(),stub!\n");
    return swap_interval;
}

/**
 * X11DRV_wglSwapIntervalEXT
 *
 * WGL_EXT_swap_control: wglSwapIntervalEXT
 */
static BOOL WINAPI X11DRV_wglSwapIntervalEXT(int interval) {
    BOOL ret = TRUE;

    TRACE("(%d)\n", interval);
    swap_interval = interval;
    if (NULL != pglXSwapIntervalSGI) {
        wine_tsx11_lock();
        ret = !pglXSwapIntervalSGI(interval);
        wine_tsx11_unlock();
    }
    else WARN("(): GLX_SGI_swap_control extension seems not supported\n");
    return ret;
}

/**
 * X11DRV_wglAllocateMemoryNV
 *
 * WGL_NV_vertex_array_range: wglAllocateMemoryNV
 */
static void* WINAPI X11DRV_wglAllocateMemoryNV(GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority) {
    void *ret = NULL;
    TRACE("(%d, %f, %f, %f)\n", size, readfreq, writefreq, priority );

    if (pglXAllocateMemoryNV)
    {
        wine_tsx11_lock();
        ret = pglXAllocateMemoryNV(size, readfreq, writefreq, priority);
        wine_tsx11_unlock();
    }
    return ret;
}

/**
 * X11DRV_wglFreeMemoryNV
 *
 * WGL_NV_vertex_array_range: wglFreeMemoryNV
 */
static void WINAPI X11DRV_wglFreeMemoryNV(GLvoid* pointer) {
    TRACE("(%p)\n", pointer);
    if (pglXFreeMemoryNV == NULL)
        return;

    wine_tsx11_lock();
    pglXFreeMemoryNV(pointer);
    wine_tsx11_unlock();
}

/**
 * X11DRV_wglSetPixelFormatWINE
 *
 * WGL_WINE_pixel_format_passthrough: wglSetPixelFormatWINE
 * This is a WINE-specific wglSetPixelFormat which can set the pixel format multiple times.
 */
BOOL CDECL X11DRV_wglSetPixelFormatWINE(X11DRV_PDEVICE *physDev, int iPixelFormat, const PIXELFORMATDESCRIPTOR *ppfd)
{
    TRACE("(%p,%d,%p)\n", physDev, iPixelFormat, ppfd);

    if (!has_opengl()) return FALSE;

    if (physDev->current_pf == iPixelFormat) return TRUE;

    /* Relay to the core SetPixelFormat */
    TRACE("Changing iPixelFormat from %d to %d\n", physDev->current_pf, iPixelFormat);
    return internal_SetPixelFormat(physDev, iPixelFormat, ppfd);
}

/**
 * glxRequireVersion (internal)
 *
 * Check if the supported GLX version matches requiredVersion.
 */
static BOOL glxRequireVersion(int requiredVersion)
{
    /* Both requiredVersion and glXVersion[1] contains the minor GLX version */
    if(requiredVersion <= WineGLInfo.glxVersion[1])
        return TRUE;

    return FALSE;
}

static BOOL glxRequireExtension(const char *requiredExtension)
{
    if (strstr(WineGLInfo.glxExtensions, requiredExtension) == NULL) {
        return FALSE;
    }

    return TRUE;
}

static void register_extension_string(const char *ext)
{
    if (WineGLInfo.wglExtensions[0])
        strcat(WineGLInfo.wglExtensions, " ");
    strcat(WineGLInfo.wglExtensions, ext);

    TRACE("'%s'\n", ext);
}

static BOOL register_extension(const WineGLExtension * ext)
{
    int i;

    assert( WineGLExtensionListSize < MAX_EXTENSIONS );
    WineGLExtensionList[WineGLExtensionListSize++] = ext;

    register_extension_string(ext->extName);

    for (i = 0; ext->extEntryPoints[i].funcName; ++i)
        TRACE("    - '%s'\n", ext->extEntryPoints[i].funcName);

    return TRUE;
}

static const WineGLExtension WGL_internal_functions =
{
  "",
  {
    { "wglGetIntegerv", X11DRV_wglGetIntegerv },
    { "wglFinish", X11DRV_wglFinish },
    { "wglFlush", X11DRV_wglFlush },
  }
};


static const WineGLExtension WGL_ARB_create_context =
{
  "WGL_ARB_create_context",
  {
    { "wglCreateContextAttribsARB", X11DRV_wglCreateContextAttribsARB },
  }
};

static const WineGLExtension WGL_ARB_extensions_string =
{
  "WGL_ARB_extensions_string",
  {
    { "wglGetExtensionsStringARB", X11DRV_wglGetExtensionsStringARB },
  }
};

static const WineGLExtension WGL_ARB_make_current_read =
{
  "WGL_ARB_make_current_read",
  {
    { "wglGetCurrentReadDCARB", X11DRV_wglGetCurrentReadDCARB },
    { "wglMakeContextCurrentARB", X11DRV_wglMakeContextCurrentARB },
  }
};

static const WineGLExtension WGL_ARB_multisample =
{
  "WGL_ARB_multisample",
};

static const WineGLExtension WGL_ARB_pbuffer =
{
  "WGL_ARB_pbuffer",
  {
    { "wglCreatePbufferARB", X11DRV_wglCreatePbufferARB },
    { "wglDestroyPbufferARB", X11DRV_wglDestroyPbufferARB },
    { "wglGetPbufferDCARB", X11DRV_wglGetPbufferDCARB },
    { "wglQueryPbufferARB", X11DRV_wglQueryPbufferARB },
    { "wglReleasePbufferDCARB", X11DRV_wglReleasePbufferDCARB },
    { "wglSetPbufferAttribARB", X11DRV_wglSetPbufferAttribARB },
  }
};

static const WineGLExtension WGL_ARB_pixel_format =
{
  "WGL_ARB_pixel_format",
  {
    { "wglChoosePixelFormatARB", X11DRV_wglChoosePixelFormatARB },
    { "wglGetPixelFormatAttribfvARB", X11DRV_wglGetPixelFormatAttribfvARB },
    { "wglGetPixelFormatAttribivARB", X11DRV_wglGetPixelFormatAttribivARB },
  }
};

static const WineGLExtension WGL_ARB_render_texture =
{
  "WGL_ARB_render_texture",
  {
    { "wglBindTexImageARB", X11DRV_wglBindTexImageARB },
    { "wglReleaseTexImageARB", X11DRV_wglReleaseTexImageARB },
  }
};

static const WineGLExtension WGL_EXT_extensions_string =
{
  "WGL_EXT_extensions_string",
  {
    { "wglGetExtensionsStringEXT", X11DRV_wglGetExtensionsStringEXT },
  }
};

static const WineGLExtension WGL_EXT_swap_control =
{
  "WGL_EXT_swap_control",
  {
    { "wglSwapIntervalEXT", X11DRV_wglSwapIntervalEXT },
    { "wglGetSwapIntervalEXT", X11DRV_wglGetSwapIntervalEXT },
  }
};

static const WineGLExtension WGL_NV_vertex_array_range =
{
  "WGL_NV_vertex_array_range",
  {
    { "wglAllocateMemoryNV", X11DRV_wglAllocateMemoryNV },
    { "wglFreeMemoryNV", X11DRV_wglFreeMemoryNV },
  }
};

static const WineGLExtension WGL_WINE_pixel_format_passthrough =
{
  "WGL_WINE_pixel_format_passthrough",
  {
    { "wglSetPixelFormatWINE", X11DRV_wglSetPixelFormatWINE },
  }
};

/**
 * X11DRV_WineGL_LoadExtensions
 */
static void X11DRV_WineGL_LoadExtensions(void)
{
    WineGLInfo.wglExtensions[0] = 0;

    /* Load Wine internal functions */
    register_extension(&WGL_internal_functions);

    /* ARB Extensions */

    if(glxRequireExtension("GLX_ARB_create_context"))
    {
        register_extension(&WGL_ARB_create_context);

        if(glxRequireExtension("GLX_ARB_create_context_profile"))
            register_extension_string("WGL_ARB_create_context_profile");
    }

    if(glxRequireExtension("GLX_ARB_fbconfig_float"))
    {
        register_extension_string("WGL_ARB_pixel_format_float");
        register_extension_string("WGL_ATI_pixel_format_float");
    }

    register_extension(&WGL_ARB_extensions_string);

    if (glxRequireVersion(3))
        register_extension(&WGL_ARB_make_current_read);

    if (glxRequireExtension("GLX_ARB_multisample"))
        register_extension(&WGL_ARB_multisample);

    /* In general pbuffer functionality requires support in the X-server. The functionality is
     * available either when the GLX_SGIX_pbuffer is present or when the GLX server version is 1.3.
     * All display drivers except for Nvidia's use the GLX module from Xfree86/Xorg which only
     * supports GLX 1.2. The endresult is that only Nvidia's drivers support pbuffers.
     *
     * The only other drive which has pbuffer support is Ati's FGLRX driver. They provide clientside GLX 1.3 support
     * without support in the X-server (which other Mesa based drivers require).
     *
     * Support pbuffers when the GLX version is 1.3 and GLX_SGIX_pbuffer is available. Further pbuffers can
     * also be supported when GLX_ATI_render_texture is available. This extension depends on pbuffers, so when it
     * is available pbuffers must be available too. */
    if ( (glxRequireVersion(3) && glxRequireExtension("GLX_SGIX_pbuffer")) || glxRequireExtension("GLX_ATI_render_texture"))
        register_extension(&WGL_ARB_pbuffer);

    register_extension(&WGL_ARB_pixel_format);

    /* Support WGL_ARB_render_texture when there's support or pbuffer based emulation */
    if (glxRequireExtension("GLX_ATI_render_texture") ||
        glxRequireExtension("GLX_ARB_render_texture") ||
        (glxRequireVersion(3) && glxRequireExtension("GLX_SGIX_pbuffer") && use_render_texture_emulation))
    {
        register_extension(&WGL_ARB_render_texture);

        /* The WGL version of GLX_NV_float_buffer requires render_texture */
        if(glxRequireExtension("GLX_NV_float_buffer"))
            register_extension_string("WGL_NV_float_buffer");

        /* Again there's no GLX equivalent for this extension, so depend on the required GL extension */
        if(strstr(WineGLInfo.glExtensions, "GL_NV_texture_rectangle") != NULL)
            register_extension_string("WGL_NV_texture_rectangle");
    }

    /* EXT Extensions */

    register_extension(&WGL_EXT_extensions_string);

    /* Load this extension even when it isn't backed by a GLX extension because it is has been around for ages.
     * Games like Call of Duty and K.O.T.O.R. rely on it. Further our emulation is good enough. */
    register_extension(&WGL_EXT_swap_control);

    if(glxRequireExtension("GLX_EXT_framebuffer_sRGB"))
        register_extension_string("WGL_EXT_framebuffer_sRGB");

    if(glxRequireExtension("GLX_EXT_fbconfig_packed_float"))
        register_extension_string("WGL_EXT_pixel_format_packed_float");

    /* The OpenGL extension GL_NV_vertex_array_range adds wgl/glX functions which aren't exported as 'real' wgl/glX extensions. */
    if(strstr(WineGLInfo.glExtensions, "GL_NV_vertex_array_range") != NULL)
        register_extension(&WGL_NV_vertex_array_range);

    /* WINE-specific WGL Extensions */

    /* In WineD3D we need the ability to set the pixel format more than once (e.g. after a device reset).
     * The default wglSetPixelFormat doesn't allow this, so add our own which allows it.
     */
    register_extension(&WGL_WINE_pixel_format_passthrough);
}


Drawable get_glxdrawable(X11DRV_PDEVICE *physDev)
{
    Drawable ret;

    if(physDev->bitmap)
    {
        if (physDev->bitmap->hbitmap == BITMAP_stock_phys_bitmap.hbitmap)
            ret = physDev->drawable; /* PBuffer */
        else
            ret = physDev->bitmap->glxpixmap;
    }
    else if(physDev->gl_drawable)
        ret = physDev->gl_drawable;
    else
        ret = physDev->drawable;
    return ret;
}

BOOL destroy_glxpixmap(Display *display, XID glxpixmap)
{
    wine_tsx11_lock(); 
    pglXDestroyGLXPixmap(display, glxpixmap);
    wine_tsx11_unlock(); 
    return TRUE;
}

/**
 * X11DRV_SwapBuffers
 *
 * Swap the buffers of this DC
 */
BOOL CDECL X11DRV_SwapBuffers(X11DRV_PDEVICE *physDev)
{
  GLXDrawable drawable;
  Wine_GLContext *ctx = NtCurrentTeb()->glContext;

  if (!has_opengl()) return FALSE;

  TRACE("(%p)\n", physDev);

  drawable = get_glxdrawable(physDev);

  wine_tsx11_lock();
  sync_context(ctx);
  if(physDev->pixmap) {
      if(pglXCopySubBufferMESA) {
          int w = physDev->dc_rect.right - physDev->dc_rect.left;
          int h = physDev->dc_rect.bottom - physDev->dc_rect.top;

          /* (glX)SwapBuffers has an implicit glFlush effect, however
           * GLX_MESA_copy_sub_buffer doesn't. Make sure GL is flushed before
           * copying */
          pglFlush();
          if(w > 0 && h > 0)
              pglXCopySubBufferMESA(gdi_display, drawable, 0, 0, w, h);
      }
      else
          pglXSwapBuffers(gdi_display, drawable);
  }
  else
      pglXSwapBuffers(gdi_display, drawable);

  flush_gl_drawable(physDev);
  wine_tsx11_unlock();

  /* FPS support */
  if (TRACE_ON(fps))
  {
      static long prev_time, start_time;
      static unsigned long frames, frames_total;

      DWORD time = GetTickCount();
      frames++;
      frames_total++;
      /* every 1.5 seconds */
      if (time - prev_time > 1500) {
          TRACE_(fps)("@ approx %.2ffps, total %.2ffps\n",
                      1000.0*frames/(time - prev_time), 1000.0*frames_total/(time - start_time));
          prev_time = time;
          frames = 0;
          if(start_time == 0) start_time = time;
      }
  }

  return TRUE;
}

XVisualInfo *visual_from_fbconfig_id( XID fbconfig_id )
{
    WineGLPixelFormat *fmt;
    XVisualInfo *ret;

    fmt = ConvertPixelFormatGLXtoWGL(gdi_display, fbconfig_id, 0 /* no flags */);
    if(fmt == NULL)
        return NULL;

    wine_tsx11_lock();
    ret = pglXGetVisualFromFBConfig(gdi_display, fmt->fbconfig);
    wine_tsx11_unlock();
    return ret;
}

#else  /* no OpenGL includes */

void X11DRV_OpenGL_Cleanup(void)
{
}

static inline void opengl_error(void)
{
    static int warned;
    if (!warned++) ERR("No OpenGL support compiled in.\n");
}

int pixelformat_from_fbconfig_id(XID fbconfig_id)
{
    return 0;
}

void mark_drawable_dirty(Drawable old, Drawable new)
{
}

void flush_gl_drawable(X11DRV_PDEVICE *physDev)
{
}

Drawable create_glxpixmap(Display *display, XVisualInfo *vis, Pixmap parent)
{
    return 0;
}

/***********************************************************************
 *		ChoosePixelFormat (X11DRV.@)
 */
int CDECL X11DRV_ChoosePixelFormat(X11DRV_PDEVICE *physDev,
                                     const PIXELFORMATDESCRIPTOR *ppfd) {
  opengl_error();
  return 0;
}

/***********************************************************************
 *		DescribePixelFormat (X11DRV.@)
 */
int CDECL X11DRV_DescribePixelFormat(X11DRV_PDEVICE *physDev,
                                     int iPixelFormat,
                                     UINT nBytes,
                                     PIXELFORMATDESCRIPTOR *ppfd) {
  opengl_error();
  return 0;
}

/***********************************************************************
 *		GetPixelFormat (X11DRV.@)
 */
int CDECL X11DRV_GetPixelFormat(X11DRV_PDEVICE *physDev) {
  opengl_error();
  return 0;
}

/***********************************************************************
 *		SetPixelFormat (X11DRV.@)
 */
BOOL CDECL X11DRV_SetPixelFormat(X11DRV_PDEVICE *physDev,
                                   int iPixelFormat,
                                   const PIXELFORMATDESCRIPTOR *ppfd) {
  opengl_error();
  return FALSE;
}

/***********************************************************************
 *		SwapBuffers (X11DRV.@)
 */
BOOL CDECL X11DRV_SwapBuffers(X11DRV_PDEVICE *physDev) {
  opengl_error();
  return FALSE;
}

/**
 * X11DRV_wglCopyContext
 *
 * For OpenGL32 wglCopyContext.
 */
BOOL CDECL X11DRV_wglCopyContext(HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask) {
    opengl_error();
    return FALSE;
}

/**
 * X11DRV_wglCreateContext
 *
 * For OpenGL32 wglCreateContext.
 */
HGLRC CDECL X11DRV_wglCreateContext(X11DRV_PDEVICE *physDev) {
    opengl_error();
    return NULL;
}

/**
 * X11DRV_wglCreateContextAttribsARB
 *
 * WGL_ARB_create_context: wglCreateContextAttribsARB
 */
HGLRC CDECL X11DRV_wglCreateContextAttribsARB(X11DRV_PDEVICE *physDev, HGLRC hShareContext, const int* attribList)
{
    opengl_error();
    return NULL;
}

/**
 * X11DRV_wglDeleteContext
 *
 * For OpenGL32 wglDeleteContext.
 */
BOOL CDECL X11DRV_wglDeleteContext(HGLRC hglrc) {
    opengl_error();
    return FALSE;
}

/**
 * X11DRV_wglGetProcAddress
 *
 * For OpenGL32 wglGetProcAddress.
 */
PROC CDECL X11DRV_wglGetProcAddress(LPCSTR lpszProc) {
    opengl_error();
    return NULL;
}

HDC CDECL X11DRV_wglGetPbufferDCARB(X11DRV_PDEVICE *hDevice, void *hPbuffer)
{
    opengl_error();
    return NULL;
}

BOOL CDECL X11DRV_wglMakeContextCurrentARB(X11DRV_PDEVICE* hDrawDev, X11DRV_PDEVICE* hReadDev, HGLRC hglrc) {
    opengl_error();
    return FALSE;
}

/**
 * X11DRV_wglMakeCurrent
 *
 * For OpenGL32 wglMakeCurrent.
 */
BOOL CDECL X11DRV_wglMakeCurrent(X11DRV_PDEVICE *physDev, HGLRC hglrc) {
    opengl_error();
    return FALSE;
}

/**
 * X11DRV_wglShareLists
 *
 * For OpenGL32 wglShareLists.
 */
BOOL CDECL X11DRV_wglShareLists(HGLRC hglrc1, HGLRC hglrc2) {
    opengl_error();
    return FALSE;
}

/**
 * X11DRV_wglUseFontBitmapsA
 *
 * For OpenGL32 wglUseFontBitmapsA.
 */
BOOL CDECL X11DRV_wglUseFontBitmapsA(X11DRV_PDEVICE *physDev, DWORD first, DWORD count, DWORD listBase)
{
    opengl_error();
    return FALSE;
}

/**
 * X11DRV_wglUseFontBitmapsW
 *
 * For OpenGL32 wglUseFontBitmapsW.
 */
BOOL CDECL X11DRV_wglUseFontBitmapsW(X11DRV_PDEVICE *physDev, DWORD first, DWORD count, DWORD listBase)
{
    opengl_error();
    return FALSE;
}

/**
 * X11DRV_wglSetPixelFormatWINE
 *
 * WGL_WINE_pixel_format_passthrough: wglSetPixelFormatWINE
 * This is a WINE-specific wglSetPixelFormat which can set the pixel format multiple times.
 */
BOOL CDECL X11DRV_wglSetPixelFormatWINE(X11DRV_PDEVICE *physDev, int iPixelFormat, const PIXELFORMATDESCRIPTOR *ppfd)
{
    opengl_error();
    return FALSE;
}

Drawable get_glxdrawable(X11DRV_PDEVICE *physDev)
{
    return 0;
}

BOOL destroy_glxpixmap(Display *display, XID glxpixmap)
{
    return FALSE;
}

XVisualInfo *visual_from_fbconfig_id( XID fbconfig_id )
{
    return NULL;
}

#endif /* defined(SONAME_LIBGL) */
