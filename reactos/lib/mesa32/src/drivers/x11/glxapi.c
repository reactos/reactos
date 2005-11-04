
/*
 * Mesa 3-D graphics library
 * Version:  5.1
 * 
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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


/*
 * This is the GLX API dispatcher.  Calls to the glX* functions are
 * either routed to the real GLX encoders or to Mesa's pseudo-GLX functions.
 * See the glxapi.h file for more details.
 */


#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "glapi.h"
#include "glxapi.h"


extern struct _glxapi_table *_real_GetGLXDispatchTable(void);
extern struct _glxapi_table *_mesa_GetGLXDispatchTable(void);


struct display_dispatch {
   Display *Dpy;
   struct _glxapi_table *Table;
   struct display_dispatch *Next;
};

static struct display_dispatch *DispatchList = NULL;


/* Display -> Dispatch caching */
static Display *prevDisplay = NULL;
static struct _glxapi_table *prevTable = NULL;


static struct _glxapi_table *
get_dispatch(Display *dpy)
{
   if (!dpy)
      return NULL;

   /* search list of display/dispatch pairs for this display */
   {
      const struct display_dispatch *d = DispatchList;
      while (d) {
         if (d->Dpy == dpy) {
            prevDisplay = dpy;
            prevTable = d->Table;
            return d->Table;  /* done! */
         }
         d = d->Next;
      }
   }

   /* A new display, determine if we should use real GLX
    * or Mesa's pseudo-GLX.
    */
   {
      struct _glxapi_table *t = NULL;

#ifdef GLX_BUILT_IN_XMESA
      if (!getenv("LIBGL_FORCE_XMESA")) {
         int ignore;
         if (XQueryExtension( dpy, "GLX", &ignore, &ignore, &ignore )) {
            /* the X server has the GLX extension */
            t = _real_GetGLXDispatchTable();
         }
      }
#endif

      if (!t) {
         /* Fallback to Mesa with Xlib driver */
#ifdef GLX_BUILT_IN_XMESA
         if (getenv("LIBGL_DEBUG")) {
            fprintf(stderr,
                    "libGL: server %s lacks the GLX extension.",
                    dpy->display_name);
            fprintf(stderr, " Using Mesa Xlib renderer.\n");
         }
#endif
         t = _mesa_GetGLXDispatchTable();
         assert(t);  /* this has to work */
      }

      if (t) {
         struct display_dispatch *d;
         d = (struct display_dispatch *) malloc(sizeof(struct display_dispatch));
         if (d) {
            d->Dpy = dpy;
            d->Table = t;
            /* insert at head of list */
            d->Next = DispatchList;
            DispatchList = d;
            /* update cache */
            prevDisplay = dpy;
            prevTable = t;
            return t;
         }
      }
   }

   /* If we get here that means we can't use real GLX on this display
    * and the Mesa pseudo-GLX software renderer wasn't compiled in.
    * Or, we ran out of memory!
    */
   return NULL;
}


#define GET_DISPATCH(DPY, TABLE)	\
   if (DPY == prevDisplay) {		\
      TABLE = prevTable;		\
   }					\
   else if (!DPY) {			\
      TABLE = NULL;			\
   }					\
   else {				\
      TABLE = get_dispatch(DPY);	\
   }

   


/* Set by glXMakeCurrent() and glXMakeContextCurrent() only */
#ifndef GLX_BUILT_IN_XMESA
static GLXContext CurrentContext = 0;
#define __glXGetCurrentContext() CurrentContext;
#endif


/*
 * GLX API entrypoints
 */

/*** GLX_VERSION_1_0 ***/

XVisualInfo *glXChooseVisual(Display *dpy, int screen, int *list)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return NULL;
   return (t->ChooseVisual)(dpy, screen, list);
}


void glXCopyContext(Display *dpy, GLXContext src, GLXContext dst, unsigned long mask)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->CopyContext)(dpy, src, dst, mask);
}


GLXContext glXCreateContext(Display *dpy, XVisualInfo *visinfo, GLXContext shareList, Bool direct)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreateContext)(dpy, visinfo, shareList, direct);
}


GLXPixmap glXCreateGLXPixmap(Display *dpy, XVisualInfo *visinfo, Pixmap pixmap)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreateGLXPixmap)(dpy, visinfo, pixmap);
}


void glXDestroyContext(Display *dpy, GLXContext ctx)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->DestroyContext)(dpy, ctx);
}


void glXDestroyGLXPixmap(Display *dpy, GLXPixmap pixmap)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->DestroyGLXPixmap)(dpy, pixmap);
}


int glXGetConfig(Display *dpy, XVisualInfo *visinfo, int attrib, int *value)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return GLX_NO_EXTENSION;
   return (t->GetConfig)(dpy, visinfo, attrib, value);
}


#ifdef GLX_BUILT_IN_XMESA
/* Use real libGL's glXGetCurrentContext() function */
#else
/* stand-alone Mesa */
GLXContext glXGetCurrentContext(void)
{
   return CurrentContext;
}
#endif


#ifdef GLX_BUILT_IN_XMESA
/* Use real libGL's glXGetCurrentContext() function */
#else
/* stand-alone Mesa */
GLXDrawable glXGetCurrentDrawable(void)
{
   __GLXcontext *gc = (__GLXcontext *) glXGetCurrentContext();
   return gc ? gc->currentDrawable : 0;
}
#endif


Bool glXIsDirect(Display *dpy, GLXContext ctx)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   return (t->IsDirect)(dpy, ctx);
}


Bool glXMakeCurrent(Display *dpy, GLXDrawable drawable, GLXContext ctx)
{
   Bool b;
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t) {
      return False;
   }
   b = (*t->MakeCurrent)(dpy, drawable, ctx);
#ifndef  GLX_BUILT_IN_XMESA
   if (b) {
      CurrentContext = ctx;
   }
#endif
   return b;
}


Bool glXQueryExtension(Display *dpy, int *errorb, int *event)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   return (t->QueryExtension)(dpy, errorb, event);
}


Bool glXQueryVersion(Display *dpy, int *maj, int *min)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   return (t->QueryVersion)(dpy, maj, min);
}


void glXSwapBuffers(Display *dpy, GLXDrawable drawable)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->SwapBuffers)(dpy, drawable);
}


void glXUseXFont(Font font, int first, int count, int listBase)
{
   struct _glxapi_table *t;
   Display *dpy = glXGetCurrentDisplay();
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->UseXFont)(font, first, count, listBase);
}


void glXWaitGL(void)
{
   struct _glxapi_table *t;
   Display *dpy = glXGetCurrentDisplay();
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->WaitGL)();
}


void glXWaitX(void)
{
   struct _glxapi_table *t;
   Display *dpy = glXGetCurrentDisplay();
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->WaitX)();
}



/*** GLX_VERSION_1_1 ***/

const char *glXGetClientString(Display *dpy, int name)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return NULL;
   return (t->GetClientString)(dpy, name);
}


const char *glXQueryExtensionsString(Display *dpy, int screen)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return NULL;
   return (t->QueryExtensionsString)(dpy, screen);
}


const char *glXQueryServerString(Display *dpy, int screen, int name)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return NULL;
   return (t->QueryServerString)(dpy, screen, name);
}


/*** GLX_VERSION_1_2 ***/

#if !defined(GLX_BUILT_IN_XMESA)
Display *glXGetCurrentDisplay(void)
{
   /* Same code as in libGL's glxext.c */
   __GLXcontext *gc = (__GLXcontext *) glXGetCurrentContext();
   if (NULL == gc) return NULL;
   return gc->currentDpy;
}
#endif



/*** GLX_VERSION_1_3 ***/

GLXFBConfig *glXChooseFBConfig(Display *dpy, int screen, const int *attribList, int *nitems)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->ChooseFBConfig)(dpy, screen, attribList, nitems);
}


GLXContext glXCreateNewContext(Display *dpy, GLXFBConfig config, int renderType, GLXContext shareList, Bool direct)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreateNewContext)(dpy, config, renderType, shareList, direct);
}


GLXPbuffer glXCreatePbuffer(Display *dpy, GLXFBConfig config, const int *attribList)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreatePbuffer)(dpy, config, attribList);
}


GLXPixmap glXCreatePixmap(Display *dpy, GLXFBConfig config, Pixmap pixmap, const int *attribList)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreatePixmap)(dpy, config, pixmap, attribList);
}


GLXWindow glXCreateWindow(Display *dpy, GLXFBConfig config, Window win, const int *attribList)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreateWindow)(dpy, config, win, attribList);
}


void glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->DestroyPbuffer)(dpy, pbuf);
}


void glXDestroyPixmap(Display *dpy, GLXPixmap pixmap)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->DestroyPixmap)(dpy, pixmap);
}


void glXDestroyWindow(Display *dpy, GLXWindow window)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->DestroyWindow)(dpy, window);
}


#ifdef GLX_BUILT_IN_XMESA
/* Use the glXGetCurrentReadDrawable() function from libGL */
#else
GLXDrawable glXGetCurrentReadDrawable(void)
{
   __GLXcontext *gc = (__GLXcontext *) glXGetCurrentContext();
   return gc ? gc->currentReadable : 0;
}
#endif


int glXGetFBConfigAttrib(Display *dpy, GLXFBConfig config, int attribute, int *value)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return GLX_NO_EXTENSION;
   return (t->GetFBConfigAttrib)(dpy, config, attribute, value);
}


GLXFBConfig *glXGetFBConfigs(Display *dpy, int screen, int *nelements)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->GetFBConfigs)(dpy, screen, nelements);
}

void glXGetSelectedEvent(Display *dpy, GLXDrawable drawable, unsigned long *mask)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->GetSelectedEvent)(dpy, drawable, mask);
}


XVisualInfo *glXGetVisualFromFBConfig(Display *dpy, GLXFBConfig config)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return NULL;
   return (t->GetVisualFromFBConfig)(dpy, config);
}


Bool glXMakeContextCurrent(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx)
{
   Bool b;
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   b = (t->MakeContextCurrent)(dpy, draw, read, ctx);
#ifndef GLX_BUILT_IN_XMESA
   if (b) {
      CurrentContext = ctx;
   }
#endif
   return b;
}


int glXQueryContext(Display *dpy, GLXContext ctx, int attribute, int *value)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   assert(t);
   if (!t)
      return 0; /* XXX correct? */
   return (t->QueryContext)(dpy, ctx, attribute, value);
}


void glXQueryDrawable(Display *dpy, GLXDrawable draw, int attribute, unsigned int *value)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->QueryDrawable)(dpy, draw, attribute, value);
}


void glXSelectEvent(Display *dpy, GLXDrawable drawable, unsigned long mask)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->SelectEvent)(dpy, drawable, mask);
}



/*** GLX_SGI_swap_control ***/

int glXSwapIntervalSGI(int interval)
{
   struct _glxapi_table *t;
   Display *dpy = glXGetCurrentDisplay();
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->SwapIntervalSGI)(interval);
}



/*** GLX_SGI_video_sync ***/

int glXGetVideoSyncSGI(unsigned int *count)
{
   struct _glxapi_table *t;
   Display *dpy = glXGetCurrentDisplay();
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->GetVideoSyncSGI)(count);
}

int glXWaitVideoSyncSGI(int divisor, int remainder, unsigned int *count)
{
   struct _glxapi_table *t;
   Display *dpy = glXGetCurrentDisplay();
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->WaitVideoSyncSGI)(divisor, remainder, count);
}



/*** GLX_SGI_make_current_read ***/

Bool glXMakeCurrentReadSGI(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->MakeCurrentReadSGI)(dpy, draw, read, ctx);
}

#ifdef GLX_BUILT_IN_XMESA
/* Use glXGetCurrentReadDrawableSGI() from libGL */
#else
/* stand-alone Mesa */
GLXDrawable glXGetCurrentReadDrawableSGI(void)
{
   return glXGetCurrentReadDrawable();
}
#endif


#if defined(_VL_H)

GLXVideoSourceSGIX glXCreateGLXVideoSourceSGIX(Display *dpy, int screen, VLServer server, VLPath path, int nodeClass, VLNode drainNode)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreateGLXVideoSourceSGIX)(dpy, screen, server, path, nodeClass, drainNode);
}

void glXDestroyGLXVideoSourceSGIX(Display *dpy, GLXVideoSourceSGIX src)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->DestroyGLXVideoSourceSGIX)(dpy, src);
}

#endif


/*** GLX_EXT_import_context ***/

void glXFreeContextEXT(Display *dpy, GLXContext context)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->FreeContextEXT)(dpy, context);
}

#ifdef GLX_BUILT_IN_XMESA
/* Use real libGL's glXGetContextIDEXT() function */
#else
/* stand-alone Mesa */
GLXContextID glXGetContextIDEXT(const GLXContext context)
{
   return ((__GLXcontext *) context)->xid;
}
#endif

#ifdef GLX_BUILT_IN_XMESA
/* Use real libGL's glXGetCurrentDisplayEXT() function */
#else
/* stand-alone Mesa */
Display *glXGetCurrentDisplayEXT(void)
{
   return glXGetCurrentDisplay();
}
#endif

GLXContext glXImportContextEXT(Display *dpy, GLXContextID contextID)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->ImportContextEXT)(dpy, contextID);
}

int glXQueryContextInfoEXT(Display *dpy, GLXContext context, int attribute,int *value)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;  /* XXX ok? */
   return (t->QueryContextInfoEXT)(dpy, context, attribute, value);
}



/*** GLX_SGIX_fbconfig ***/

int glXGetFBConfigAttribSGIX(Display *dpy, GLXFBConfigSGIX config, int attribute, int *value)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->GetFBConfigAttribSGIX)(dpy, config, attribute, value);
}

GLXFBConfigSGIX *glXChooseFBConfigSGIX(Display *dpy, int screen, int *attrib_list, int *nelements)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->ChooseFBConfigSGIX)(dpy, screen, attrib_list, nelements);
}

GLXPixmap glXCreateGLXPixmapWithConfigSGIX(Display *dpy, GLXFBConfigSGIX config, Pixmap pixmap)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreateGLXPixmapWithConfigSGIX)(dpy, config, pixmap);
}

GLXContext glXCreateContextWithConfigSGIX(Display *dpy, GLXFBConfigSGIX config, int render_type, GLXContext share_list, Bool direct)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreateContextWithConfigSGIX)(dpy, config, render_type, share_list, direct);
}

XVisualInfo * glXGetVisualFromFBConfigSGIX(Display *dpy, GLXFBConfigSGIX config)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->GetVisualFromFBConfigSGIX)(dpy, config);
}

GLXFBConfigSGIX glXGetFBConfigFromVisualSGIX(Display *dpy, XVisualInfo *vis)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->GetFBConfigFromVisualSGIX)(dpy, vis);
}



/*** GLX_SGIX_pbuffer ***/

GLXPbufferSGIX glXCreateGLXPbufferSGIX(Display *dpy, GLXFBConfigSGIX config, unsigned int width, unsigned int height, int *attrib_list)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreateGLXPbufferSGIX)(dpy, config, width, height, attrib_list);
}

void glXDestroyGLXPbufferSGIX(Display *dpy, GLXPbufferSGIX pbuf)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->DestroyGLXPbufferSGIX)(dpy, pbuf);
}

int glXQueryGLXPbufferSGIX(Display *dpy, GLXPbufferSGIX pbuf, int attribute, unsigned int *value)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->QueryGLXPbufferSGIX)(dpy, pbuf, attribute, value);
}

void glXSelectEventSGIX(Display *dpy, GLXDrawable drawable, unsigned long mask)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->SelectEventSGIX)(dpy, drawable, mask);
}

void glXGetSelectedEventSGIX(Display *dpy, GLXDrawable drawable, unsigned long *mask)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->GetSelectedEventSGIX)(dpy, drawable, mask);
}



/*** GLX_SGI_cushion ***/

void glXCushionSGI(Display *dpy, Window win, float cushion)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->CushionSGI)(dpy, win, cushion);
}



/*** GLX_SGIX_video_resize ***/

int glXBindChannelToWindowSGIX(Display *dpy, int screen, int channel , Window window)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->BindChannelToWindowSGIX)(dpy, screen, channel, window);
}

int glXChannelRectSGIX(Display *dpy, int screen, int channel, int x, int y, int w, int h)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->ChannelRectSGIX)(dpy, screen, channel, x, y, w, h);
}

int glXQueryChannelRectSGIX(Display *dpy, int screen, int channel, int *x, int *y, int *w, int *h)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->QueryChannelRectSGIX)(dpy, screen, channel, x, y, w, h);
}

int glXQueryChannelDeltasSGIX(Display *dpy, int screen, int channel, int *dx, int *dy, int *dw, int *dh)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->QueryChannelDeltasSGIX)(dpy, screen, channel, dx, dy, dw, dh);
}

int glXChannelRectSyncSGIX(Display *dpy, int screen, int channel, GLenum synctype)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->ChannelRectSyncSGIX)(dpy, screen, channel, synctype);
}



#if defined(_DM_BUFFER_H_)

Bool glXAssociateDMPbufferSGIX(Display *dpy, GLXPbufferSGIX pbuffer, DMparams *params, DMbuffer dmbuffer)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   return (t->AssociateDMPbufferSGIX)(dpy, pbuffer, params, dmbuffer);
}

#endif


/*** GLX_SGIX_swap_group ***/

void glXJoinSwapGroupSGIX(Display *dpy, GLXDrawable drawable, GLXDrawable member)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (*t->JoinSwapGroupSGIX)(dpy, drawable, member);
}


/*** GLX_SGIX_swap_barrier ***/

void glXBindSwapBarrierSGIX(Display *dpy, GLXDrawable drawable, int barrier)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (*t->BindSwapBarrierSGIX)(dpy, drawable, barrier);
}

Bool glXQueryMaxSwapBarriersSGIX(Display *dpy, int screen, int *max)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   return (*t->QueryMaxSwapBarriersSGIX)(dpy, screen, max);
}



/*** GLX_SUN_get_transparent_index ***/

Status glXGetTransparentIndexSUN(Display *dpy, Window overlay, Window underlay, long *pTransparent)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   return (*t->GetTransparentIndexSUN)(dpy, overlay, underlay, pTransparent);
}



/*** GLX_MESA_copy_sub_buffer ***/

void glXCopySubBufferMESA(Display *dpy, GLXDrawable drawable, int x, int y, int width, int height)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->CopySubBufferMESA)(dpy, drawable, x, y, width, height);
}



/*** GLX_MESA_release_buffers ***/

Bool glXReleaseBuffersMESA(Display *dpy, Window w)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   return (t->ReleaseBuffersMESA)(dpy, w);
}



/*** GLX_MESA_pixmap_colormap ***/

GLXPixmap glXCreateGLXPixmapMESA(Display *dpy, XVisualInfo *visinfo, Pixmap pixmap, Colormap cmap)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreateGLXPixmapMESA)(dpy, visinfo, pixmap, cmap);
}



/*** GLX_MESA_set_3dfx_mode ***/

Bool glXSet3DfxModeMESA(int mode)
{
   struct _glxapi_table *t;
   Display *dpy = glXGetCurrentDisplay();
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   return (t->Set3DfxModeMESA)(mode);
}



/*** GLX_NV_vertex_array_range ***/

void *
glXAllocateMemoryNV( GLsizei size,
                     GLfloat readFrequency,
                     GLfloat writeFrequency,
                     GLfloat priority )
{
   struct _glxapi_table *t;
   Display *dpy = glXGetCurrentDisplay();
   GET_DISPATCH(dpy, t);
   if (!t)
      return NULL;
   return (t->AllocateMemoryNV)(size, readFrequency, writeFrequency, priority);
}


void 
glXFreeMemoryNV( GLvoid *pointer )
{
   struct _glxapi_table *t;
   Display *dpy = glXGetCurrentDisplay();
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->FreeMemoryNV)(pointer);
}


/*** GLX_MESA_agp_offset */

GLuint
glXGetAGPOffsetMESA( const GLvoid *pointer )
{
   struct _glxapi_table *t;
   Display *dpy = glXGetCurrentDisplay();
   GET_DISPATCH(dpy, t);
   if (!t)
      return ~0;
   return (t->GetAGPOffsetMESA)(pointer);
}


/*** GLX_ARB_render_Texture ***/

Bool
glXBindTexImageARB( Display *dpy, GLXPbuffer pbuffer, int buffer )
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   return (t->BindTexImageARB)(dpy, pbuffer, buffer);
}


Bool
glXReleaseTexImageARB(Display *dpy, GLXPbuffer pbuffer, int buffer )
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   return (t->ReleaseTexImageARB)(dpy, pbuffer, buffer);
}


Bool
glXDrawableAttribARB( Display *dpy, GLXDrawable draw, const int *attribList )
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   return (t->DrawableAttribARB)(dpy, draw, attribList);
}



/**********************************************************************/
/* GLX API management functions                                       */
/**********************************************************************/


const char *
_glxapi_get_version(void)
{
   return "1.3";
}


/*
 * Return array of extension strings.
 */
const char **
_glxapi_get_extensions(void)
{
   static const char *extensions[] = {
#ifdef GLX_EXT_import_context
      "GLX_EXT_import_context",
#endif
#ifdef GLX_SGI_video_sync
      "GLX_SGI_video_sync",
#endif
#ifdef GLX_MESA_copy_sub_buffer
      "GLX_MESA_copy_sub_buffer",
#endif
#ifdef GLX_MESA_release_buffers
      "GLX_MESA_release_buffers",
#endif
#ifdef GLX_MESA_pixmap_colormap
      "GLX_MESA_pixmap_colormap",
#endif
#ifdef GLX_MESA_set_3dfx_mode
      "GLX_MESA_set_3dfx_mode",
#endif
#ifdef GLX_SGIX_fbconfig
      "GLX_SGIX_fbconfig",
#endif
#ifdef GLX_SGIX_pbuffer
      "GLX_SGIX_pbuffer",
#endif
#ifdef GLX_ARB_render_texture
      "GLX_ARB_render_texture",
#endif
      NULL
   };
   return extensions;
}


/*
 * Return size of the GLX dispatch table, in entries, not bytes.
 */
GLuint
_glxapi_get_dispatch_table_size(void)
{
   return sizeof(struct _glxapi_table) / sizeof(void *);
}


static int
generic_no_op_func(void)
{
   return 0;
}


/*
 * Initialize all functions in given dispatch table to be no-ops
 */
void
_glxapi_set_no_op_table(struct _glxapi_table *t)
{
   GLuint n = _glxapi_get_dispatch_table_size();
   GLuint i;
   void **dispatch = (void **) t;
   for (i = 0; i < n; i++) {
      dispatch[i] = (void *) generic_no_op_func;
   }
}


struct name_address_pair {
   const char *Name;
   GLvoid *Address;
};

static struct name_address_pair GLX_functions[] = {
   /*** GLX_VERSION_1_0 ***/
   { "glXChooseVisual", (GLvoid *) glXChooseVisual },
   { "glXCopyContext", (GLvoid *) glXCopyContext },
   { "glXCreateContext", (GLvoid *) glXCreateContext },
   { "glXCreateGLXPixmap", (GLvoid *) glXCreateGLXPixmap },
   { "glXDestroyContext", (GLvoid *) glXDestroyContext },
   { "glXDestroyGLXPixmap", (GLvoid *) glXDestroyGLXPixmap },
   { "glXGetConfig", (GLvoid *) glXGetConfig },
   { "glXGetCurrentContext", (GLvoid *) glXGetCurrentContext },
   { "glXGetCurrentDrawable", (GLvoid *) glXGetCurrentDrawable },
   { "glXIsDirect", (GLvoid *) glXIsDirect },
   { "glXMakeCurrent", (GLvoid *) glXMakeCurrent },
   { "glXQueryExtension", (GLvoid *) glXQueryExtension },
   { "glXQueryVersion", (GLvoid *) glXQueryVersion },
   { "glXSwapBuffers", (GLvoid *) glXSwapBuffers },
   { "glXUseXFont", (GLvoid *) glXUseXFont },
   { "glXWaitGL", (GLvoid *) glXWaitGL },
   { "glXWaitX", (GLvoid *) glXWaitX },

   /*** GLX_VERSION_1_1 ***/
   { "glXGetClientString", (GLvoid *) glXGetClientString },
   { "glXQueryExtensionsString", (GLvoid *) glXQueryExtensionsString },
   { "glXQueryServerString", (GLvoid *) glXQueryServerString },

   /*** GLX_VERSION_1_2 ***/
   { "glXGetCurrentDisplay", (GLvoid *) glXGetCurrentDisplay },

   /*** GLX_VERSION_1_3 ***/
   { "glXChooseFBConfig", (GLvoid *) glXChooseFBConfig },
   { "glXCreateNewContext", (GLvoid *) glXCreateNewContext },
   { "glXCreatePbuffer", (GLvoid *) glXCreatePbuffer },
   { "glXCreatePixmap", (GLvoid *) glXCreatePixmap },
   { "glXCreateWindow", (GLvoid *) glXCreateWindow },
   { "glXDestroyPbuffer", (GLvoid *) glXDestroyPbuffer },
   { "glXDestroyPixmap", (GLvoid *) glXDestroyPixmap },
   { "glXDestroyWindow", (GLvoid *) glXDestroyWindow },
   { "glXGetCurrentReadDrawable", (GLvoid *) glXGetCurrentReadDrawable },
   { "glXGetFBConfigAttrib", (GLvoid *) glXGetFBConfigAttrib },
   { "glXGetFBConfigs", (GLvoid *) glXGetFBConfigs },
   { "glXGetSelectedEvent", (GLvoid *) glXGetSelectedEvent },
   { "glXGetVisualFromFBConfig", (GLvoid *) glXGetVisualFromFBConfig },
   { "glXMakeContextCurrent", (GLvoid *) glXMakeContextCurrent },
   { "glXQueryContext", (GLvoid *) glXQueryContext },
   { "glXQueryDrawable", (GLvoid *) glXQueryDrawable },
   { "glXSelectEvent", (GLvoid *) glXSelectEvent },

   /*** GLX_VERSION_1_4 ***/
   { "glXGetProcAddress", (GLvoid *) glXGetProcAddress },

   /*** GLX_SGI_swap_control ***/
   { "glXSwapIntervalSGI", (GLvoid *) glXSwapIntervalSGI },

   /*** GLX_SGI_video_sync ***/
   { "glXGetVideoSyncSGI", (GLvoid *) glXGetVideoSyncSGI },
   { "glXWaitVideoSyncSGI", (GLvoid *) glXWaitVideoSyncSGI },

   /*** GLX_SGI_make_current_read ***/
   { "glXMakeCurrentReadSGI", (GLvoid *) glXMakeCurrentReadSGI },
   { "glXGetCurrentReadDrawableSGI", (GLvoid *) glXGetCurrentReadDrawableSGI },

   /*** GLX_SGIX_video_source ***/
#if defined(_VL_H)
   { "glXCreateGLXVideoSourceSGIX", (GLvoid *) glXCreateGLXVideoSourceSGIX },
   { "glXDestroyGLXVideoSourceSGIX", (GLvoid *) glXDestroyGLXVideoSourceSGIX },
#endif

   /*** GLX_EXT_import_context ***/
   { "glXFreeContextEXT", (GLvoid *) glXFreeContextEXT },
   { "glXGetContextIDEXT", (GLvoid *) glXGetContextIDEXT },
   { "glXGetCurrentDisplayEXT", (GLvoid *) glXGetCurrentDisplayEXT },
   { "glXImportContextEXT", (GLvoid *) glXImportContextEXT },
   { "glXQueryContextInfoEXT", (GLvoid *) glXQueryContextInfoEXT },

   /*** GLX_SGIX_fbconfig ***/
   { "glXGetFBConfigAttribSGIX", (GLvoid *) glXGetFBConfigAttribSGIX },
   { "glXChooseFBConfigSGIX", (GLvoid *) glXChooseFBConfigSGIX },
   { "glXCreateGLXPixmapWithConfigSGIX", (GLvoid *) glXCreateGLXPixmapWithConfigSGIX },
   { "glXCreateContextWithConfigSGIX", (GLvoid *) glXCreateContextWithConfigSGIX },
   { "glXGetVisualFromFBConfigSGIX", (GLvoid *) glXGetVisualFromFBConfigSGIX },
   { "glXGetFBConfigFromVisualSGIX", (GLvoid *) glXGetFBConfigFromVisualSGIX },

   /*** GLX_SGIX_pbuffer ***/
   { "glXCreateGLXPbufferSGIX", (GLvoid *) glXCreateGLXPbufferSGIX },
   { "glXDestroyGLXPbufferSGIX", (GLvoid *) glXDestroyGLXPbufferSGIX },
   { "glXQueryGLXPbufferSGIX", (GLvoid *) glXQueryGLXPbufferSGIX },
   { "glXSelectEventSGIX", (GLvoid *) glXSelectEventSGIX },
   { "glXGetSelectedEventSGIX", (GLvoid *) glXGetSelectedEventSGIX },

   /*** GLX_SGI_cushion ***/
   { "glXCushionSGI", (GLvoid *) glXCushionSGI },

   /*** GLX_SGIX_video_resize ***/
   { "glXBindChannelToWindowSGIX", (GLvoid *) glXBindChannelToWindowSGIX },
   { "glXChannelRectSGIX", (GLvoid *) glXChannelRectSGIX },
   { "glXQueryChannelRectSGIX", (GLvoid *) glXQueryChannelRectSGIX },
   { "glXQueryChannelDeltasSGIX", (GLvoid *) glXQueryChannelDeltasSGIX },
   { "glXChannelRectSyncSGIX", (GLvoid *) glXChannelRectSyncSGIX },

   /*** GLX_SGIX_dmbuffer **/
#if defined(_DM_BUFFER_H_)
   { "glXAssociateDMPbufferSGIX", (GLvoid *) glXAssociateDMPbufferSGIX },
#endif

   /*** GLX_SGIX_swap_group ***/
   { "glXJoinSwapGroupSGIX", (GLvoid *) glXJoinSwapGroupSGIX },

   /*** GLX_SGIX_swap_barrier ***/
   { "glXBindSwapBarrierSGIX", (GLvoid *) glXBindSwapBarrierSGIX },
   { "glXQueryMaxSwapBarriersSGIX", (GLvoid *) glXQueryMaxSwapBarriersSGIX },

   /*** GLX_SUN_get_transparent_index ***/
   { "glXGetTransparentIndexSUN", (GLvoid *) glXGetTransparentIndexSUN },

   /*** GLX_MESA_copy_sub_buffer ***/
   { "glXCopySubBufferMESA", (GLvoid *) glXCopySubBufferMESA },

   /*** GLX_MESA_pixmap_colormap ***/
   { "glXCreateGLXPixmapMESA", (GLvoid *) glXCreateGLXPixmapMESA },

   /*** GLX_MESA_release_buffers ***/
   { "glXReleaseBuffersMESA", (GLvoid *) glXReleaseBuffersMESA },

   /*** GLX_MESA_set_3dfx_mode ***/
   { "glXSet3DfxModeMESA", (GLvoid *) glXSet3DfxModeMESA },

   /*** GLX_ARB_get_proc_address ***/
   { "glXGetProcAddressARB", (GLvoid *) glXGetProcAddressARB },

   /*** GLX_NV_vertex_array_range ***/
   { "glXAllocateMemoryNV", (GLvoid *) glXAllocateMemoryNV },
   { "glXFreeMemoryNV", (GLvoid *) glXFreeMemoryNV },

   /*** GLX_MESA_agp_offset ***/
   { "glXGetAGPOffsetMESA", (GLvoid *) glXGetAGPOffsetMESA },

   /*** GLX_ARB_render_texture ***/
   { "glXBindTexImageARB", (GLvoid *) glXBindTexImageARB },
   { "glXReleaseTexImageARB", (GLvoid *) glXReleaseTexImageARB },
   { "glXDrawableAttribARB", (GLvoid *) glXDrawableAttribARB },

   { NULL, NULL }   /* end of list */
};



/*
 * Return address of named glX function, or NULL if not found.
 */
const GLvoid *
_glxapi_get_proc_address(const char *funcName)
{
   GLuint i;
   for (i = 0; GLX_functions[i].Name; i++) {
      if (strcmp(GLX_functions[i].Name, funcName) == 0)
         return GLX_functions[i].Address;
   }
   return NULL;
}



/*
 * This function does not get dispatched through the dispatch table
 * since it's really a "meta" function.
 */
void (*glXGetProcAddressARB(const GLubyte *procName))()
{
   typedef void (*gl_function)();
   gl_function f;

   f = (gl_function) _glxapi_get_proc_address((const char *) procName);
   if (f) {
      return f;
   }

   f = (gl_function) _glapi_get_proc_address((const char *) procName);
   return f;
}


/* GLX 1.4 */
void (*glXGetProcAddress(const GLubyte *procName))()
{
   return glXGetProcAddressARB(procName);
}
