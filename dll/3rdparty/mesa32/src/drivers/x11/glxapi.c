/*
 * Mesa 3-D graphics library
 * Version:  6.5
 * 
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
#include "glheader.h"
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
      struct _glxapi_table *t = _mesa_GetGLXDispatchTable();

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


/* Don't use the GET_DISPATCH defined in glthread.h */
#undef GET_DISPATCH

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

   


/**
 * GLX API current context.
 */
#if defined(GLX_USE_TLS)
PUBLIC __thread void * CurrentContext
    __attribute__((tls_model("initial-exec")));
#elif defined(THREADS)
static _glthread_TSD ContextTSD;         /**< Per-thread context pointer */
#else
static GLXContext CurrentContext = 0;
#endif


static void
SetCurrentContext(GLXContext c)
{
#if defined(GLX_USE_TLS)
   CurrentContext = c;
#elif defined(THREADS)
   _glthread_SetTSD(&ContextTSD, c);
#else
   CurrentContext = c;
#endif
}


/*
 * GLX API entrypoints
 */

/*** GLX_VERSION_1_0 ***/

XVisualInfo PUBLIC *
glXChooseVisual(Display *dpy, int screen, int *list)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return NULL;
   return (t->ChooseVisual)(dpy, screen, list);
}


void PUBLIC
glXCopyContext(Display *dpy, GLXContext src, GLXContext dst, unsigned long mask)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->CopyContext)(dpy, src, dst, mask);
}


GLXContext PUBLIC
glXCreateContext(Display *dpy, XVisualInfo *visinfo, GLXContext shareList, Bool direct)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreateContext)(dpy, visinfo, shareList, direct);
}


GLXPixmap PUBLIC
glXCreateGLXPixmap(Display *dpy, XVisualInfo *visinfo, Pixmap pixmap)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreateGLXPixmap)(dpy, visinfo, pixmap);
}


void PUBLIC
glXDestroyContext(Display *dpy, GLXContext ctx)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   if (glXGetCurrentContext() == ctx)
      SetCurrentContext(NULL);
   (t->DestroyContext)(dpy, ctx);
}


void PUBLIC
glXDestroyGLXPixmap(Display *dpy, GLXPixmap pixmap)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->DestroyGLXPixmap)(dpy, pixmap);
}


int PUBLIC
glXGetConfig(Display *dpy, XVisualInfo *visinfo, int attrib, int *value)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return GLX_NO_EXTENSION;
   return (t->GetConfig)(dpy, visinfo, attrib, value);
}


GLXContext PUBLIC
glXGetCurrentContext(void)
{
#if defined(GLX_USE_TLS)
   return CurrentContext;
#elif defined(THREADS)
   return (GLXContext) _glthread_GetTSD(&ContextTSD);
#else
   return CurrentContext;
#endif
}


GLXDrawable PUBLIC
glXGetCurrentDrawable(void)
{
   __GLXcontext *gc = (__GLXcontext *) glXGetCurrentContext();
   return gc ? gc->currentDrawable : 0;
}


Bool PUBLIC
glXIsDirect(Display *dpy, GLXContext ctx)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   return (t->IsDirect)(dpy, ctx);
}


Bool PUBLIC
glXMakeCurrent(Display *dpy, GLXDrawable drawable, GLXContext ctx)
{
   Bool b;
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t) {
      return False;
   }
   b = (*t->MakeCurrent)(dpy, drawable, ctx);
   if (b) {
      SetCurrentContext(ctx);
   }
   return b;
}


Bool PUBLIC
glXQueryExtension(Display *dpy, int *errorb, int *event)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   return (t->QueryExtension)(dpy, errorb, event);
}


Bool PUBLIC
glXQueryVersion(Display *dpy, int *maj, int *min)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   return (t->QueryVersion)(dpy, maj, min);
}


void PUBLIC
glXSwapBuffers(Display *dpy, GLXDrawable drawable)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->SwapBuffers)(dpy, drawable);
}


void PUBLIC
glXUseXFont(Font font, int first, int count, int listBase)
{
   struct _glxapi_table *t;
   Display *dpy = glXGetCurrentDisplay();
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->UseXFont)(font, first, count, listBase);
}


void PUBLIC
glXWaitGL(void)
{
   struct _glxapi_table *t;
   Display *dpy = glXGetCurrentDisplay();
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->WaitGL)();
}


void PUBLIC
glXWaitX(void)
{
   struct _glxapi_table *t;
   Display *dpy = glXGetCurrentDisplay();
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->WaitX)();
}



/*** GLX_VERSION_1_1 ***/

const char PUBLIC *
glXGetClientString(Display *dpy, int name)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return NULL;
   return (t->GetClientString)(dpy, name);
}


const char PUBLIC *
glXQueryExtensionsString(Display *dpy, int screen)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return NULL;
   return (t->QueryExtensionsString)(dpy, screen);
}


const char PUBLIC *
glXQueryServerString(Display *dpy, int screen, int name)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return NULL;
   return (t->QueryServerString)(dpy, screen, name);
}


/*** GLX_VERSION_1_2 ***/

Display PUBLIC *
glXGetCurrentDisplay(void)
{
   /* Same code as in libGL's glxext.c */
   __GLXcontext *gc = (__GLXcontext *) glXGetCurrentContext();
   if (NULL == gc) return NULL;
   return gc->currentDpy;
}



/*** GLX_VERSION_1_3 ***/

GLXFBConfig PUBLIC *
glXChooseFBConfig(Display *dpy, int screen, const int *attribList, int *nitems)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->ChooseFBConfig)(dpy, screen, attribList, nitems);
}


GLXContext PUBLIC
glXCreateNewContext(Display *dpy, GLXFBConfig config, int renderType, GLXContext shareList, Bool direct)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreateNewContext)(dpy, config, renderType, shareList, direct);
}


GLXPbuffer PUBLIC
glXCreatePbuffer(Display *dpy, GLXFBConfig config, const int *attribList)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreatePbuffer)(dpy, config, attribList);
}


GLXPixmap PUBLIC
glXCreatePixmap(Display *dpy, GLXFBConfig config, Pixmap pixmap, const int *attribList)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreatePixmap)(dpy, config, pixmap, attribList);
}


GLXWindow PUBLIC
glXCreateWindow(Display *dpy, GLXFBConfig config, Window win, const int *attribList)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreateWindow)(dpy, config, win, attribList);
}


void PUBLIC
glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->DestroyPbuffer)(dpy, pbuf);
}


void PUBLIC
glXDestroyPixmap(Display *dpy, GLXPixmap pixmap)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->DestroyPixmap)(dpy, pixmap);
}


void PUBLIC
glXDestroyWindow(Display *dpy, GLXWindow window)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->DestroyWindow)(dpy, window);
}


GLXDrawable PUBLIC
glXGetCurrentReadDrawable(void)
{
   __GLXcontext *gc = (__GLXcontext *) glXGetCurrentContext();
   return gc ? gc->currentReadable : 0;
}


int PUBLIC
glXGetFBConfigAttrib(Display *dpy, GLXFBConfig config, int attribute, int *value)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return GLX_NO_EXTENSION;
   return (t->GetFBConfigAttrib)(dpy, config, attribute, value);
}


GLXFBConfig PUBLIC *
glXGetFBConfigs(Display *dpy, int screen, int *nelements)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->GetFBConfigs)(dpy, screen, nelements);
}

void PUBLIC
glXGetSelectedEvent(Display *dpy, GLXDrawable drawable, unsigned long *mask)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->GetSelectedEvent)(dpy, drawable, mask);
}


XVisualInfo PUBLIC *
glXGetVisualFromFBConfig(Display *dpy, GLXFBConfig config)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return NULL;
   return (t->GetVisualFromFBConfig)(dpy, config);
}


Bool PUBLIC
glXMakeContextCurrent(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx)
{
   Bool b;
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   b = (t->MakeContextCurrent)(dpy, draw, read, ctx);
   if (b) {
      SetCurrentContext(ctx);
   }
   return b;
}


int PUBLIC
glXQueryContext(Display *dpy, GLXContext ctx, int attribute, int *value)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   assert(t);
   if (!t)
      return 0; /* XXX correct? */
   return (t->QueryContext)(dpy, ctx, attribute, value);
}


void PUBLIC
glXQueryDrawable(Display *dpy, GLXDrawable draw, int attribute, unsigned int *value)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->QueryDrawable)(dpy, draw, attribute, value);
}


void PUBLIC
glXSelectEvent(Display *dpy, GLXDrawable drawable, unsigned long mask)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->SelectEvent)(dpy, drawable, mask);
}



/*** GLX_SGI_swap_control ***/

int PUBLIC
glXSwapIntervalSGI(int interval)
{
   struct _glxapi_table *t;
   Display *dpy = glXGetCurrentDisplay();
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->SwapIntervalSGI)(interval);
}



/*** GLX_SGI_video_sync ***/

int PUBLIC
glXGetVideoSyncSGI(unsigned int *count)
{
   struct _glxapi_table *t;
   Display *dpy = glXGetCurrentDisplay();
   GET_DISPATCH(dpy, t);
   if (!t || !glXGetCurrentContext())
      return GLX_BAD_CONTEXT;
   return (t->GetVideoSyncSGI)(count);
}

int PUBLIC
glXWaitVideoSyncSGI(int divisor, int remainder, unsigned int *count)
{
   struct _glxapi_table *t;
   Display *dpy = glXGetCurrentDisplay();
   GET_DISPATCH(dpy, t);
   if (!t || !glXGetCurrentContext())
      return GLX_BAD_CONTEXT;
   return (t->WaitVideoSyncSGI)(divisor, remainder, count);
}



/*** GLX_SGI_make_current_read ***/

Bool PUBLIC
glXMakeCurrentReadSGI(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   return (t->MakeCurrentReadSGI)(dpy, draw, read, ctx);
}

GLXDrawable PUBLIC
glXGetCurrentReadDrawableSGI(void)
{
   return glXGetCurrentReadDrawable();
}


#if defined(_VL_H)

GLXVideoSourceSGIX PUBLIC
glXCreateGLXVideoSourceSGIX(Display *dpy, int screen, VLServer server, VLPath path, int nodeClass, VLNode drainNode)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreateGLXVideoSourceSGIX)(dpy, screen, server, path, nodeClass, drainNode);
}

void PUBLIC
glXDestroyGLXVideoSourceSGIX(Display *dpy, GLXVideoSourceSGIX src)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->DestroyGLXVideoSourceSGIX)(dpy, src);
}

#endif


/*** GLX_EXT_import_context ***/

void PUBLIC
glXFreeContextEXT(Display *dpy, GLXContext context)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->FreeContextEXT)(dpy, context);
}

GLXContextID PUBLIC
glXGetContextIDEXT(const GLXContext context)
{
   return ((__GLXcontext *) context)->xid;
}

Display PUBLIC *
glXGetCurrentDisplayEXT(void)
{
   return glXGetCurrentDisplay();
}

GLXContext PUBLIC
glXImportContextEXT(Display *dpy, GLXContextID contextID)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->ImportContextEXT)(dpy, contextID);
}

int PUBLIC
glXQueryContextInfoEXT(Display *dpy, GLXContext context, int attribute,int *value)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;  /* XXX ok? */
   return (t->QueryContextInfoEXT)(dpy, context, attribute, value);
}



/*** GLX_SGIX_fbconfig ***/

int PUBLIC
glXGetFBConfigAttribSGIX(Display *dpy, GLXFBConfigSGIX config, int attribute, int *value)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->GetFBConfigAttribSGIX)(dpy, config, attribute, value);
}

GLXFBConfigSGIX PUBLIC *
glXChooseFBConfigSGIX(Display *dpy, int screen, int *attrib_list, int *nelements)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->ChooseFBConfigSGIX)(dpy, screen, attrib_list, nelements);
}

GLXPixmap PUBLIC
glXCreateGLXPixmapWithConfigSGIX(Display *dpy, GLXFBConfigSGIX config, Pixmap pixmap)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreateGLXPixmapWithConfigSGIX)(dpy, config, pixmap);
}

GLXContext PUBLIC
glXCreateContextWithConfigSGIX(Display *dpy, GLXFBConfigSGIX config, int render_type, GLXContext share_list, Bool direct)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreateContextWithConfigSGIX)(dpy, config, render_type, share_list, direct);
}

XVisualInfo PUBLIC *
glXGetVisualFromFBConfigSGIX(Display *dpy, GLXFBConfigSGIX config)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->GetVisualFromFBConfigSGIX)(dpy, config);
}

GLXFBConfigSGIX PUBLIC
glXGetFBConfigFromVisualSGIX(Display *dpy, XVisualInfo *vis)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->GetFBConfigFromVisualSGIX)(dpy, vis);
}



/*** GLX_SGIX_pbuffer ***/

GLXPbufferSGIX PUBLIC
glXCreateGLXPbufferSGIX(Display *dpy, GLXFBConfigSGIX config, unsigned int width, unsigned int height, int *attrib_list)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreateGLXPbufferSGIX)(dpy, config, width, height, attrib_list);
}

void PUBLIC
glXDestroyGLXPbufferSGIX(Display *dpy, GLXPbufferSGIX pbuf)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->DestroyGLXPbufferSGIX)(dpy, pbuf);
}

int PUBLIC
glXQueryGLXPbufferSGIX(Display *dpy, GLXPbufferSGIX pbuf, int attribute, unsigned int *value)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->QueryGLXPbufferSGIX)(dpy, pbuf, attribute, value);
}

void PUBLIC
glXSelectEventSGIX(Display *dpy, GLXDrawable drawable, unsigned long mask)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->SelectEventSGIX)(dpy, drawable, mask);
}

void PUBLIC
glXGetSelectedEventSGIX(Display *dpy, GLXDrawable drawable, unsigned long *mask)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->GetSelectedEventSGIX)(dpy, drawable, mask);
}



/*** GLX_SGI_cushion ***/

void PUBLIC
glXCushionSGI(Display *dpy, Window win, float cushion)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->CushionSGI)(dpy, win, cushion);
}



/*** GLX_SGIX_video_resize ***/

int PUBLIC
glXBindChannelToWindowSGIX(Display *dpy, int screen, int channel , Window window)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->BindChannelToWindowSGIX)(dpy, screen, channel, window);
}

int PUBLIC
glXChannelRectSGIX(Display *dpy, int screen, int channel, int x, int y, int w, int h)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->ChannelRectSGIX)(dpy, screen, channel, x, y, w, h);
}

int PUBLIC
glXQueryChannelRectSGIX(Display *dpy, int screen, int channel, int *x, int *y, int *w, int *h)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->QueryChannelRectSGIX)(dpy, screen, channel, x, y, w, h);
}

int PUBLIC
glXQueryChannelDeltasSGIX(Display *dpy, int screen, int channel, int *dx, int *dy, int *dw, int *dh)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->QueryChannelDeltasSGIX)(dpy, screen, channel, dx, dy, dw, dh);
}

int PUBLIC
glXChannelRectSyncSGIX(Display *dpy, int screen, int channel, GLenum synctype)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->ChannelRectSyncSGIX)(dpy, screen, channel, synctype);
}



#if defined(_DM_BUFFER_H_)

Bool PUBLIC
glXAssociateDMPbufferSGIX(Display *dpy, GLXPbufferSGIX pbuffer, DMparams *params, DMbuffer dmbuffer)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   return (t->AssociateDMPbufferSGIX)(dpy, pbuffer, params, dmbuffer);
}

#endif


/*** GLX_SGIX_swap_group ***/

void PUBLIC
glXJoinSwapGroupSGIX(Display *dpy, GLXDrawable drawable, GLXDrawable member)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (*t->JoinSwapGroupSGIX)(dpy, drawable, member);
}


/*** GLX_SGIX_swap_barrier ***/

void PUBLIC
glXBindSwapBarrierSGIX(Display *dpy, GLXDrawable drawable, int barrier)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (*t->BindSwapBarrierSGIX)(dpy, drawable, barrier);
}

Bool PUBLIC
glXQueryMaxSwapBarriersSGIX(Display *dpy, int screen, int *max)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   return (*t->QueryMaxSwapBarriersSGIX)(dpy, screen, max);
}



/*** GLX_SUN_get_transparent_index ***/

Status PUBLIC
glXGetTransparentIndexSUN(Display *dpy, Window overlay, Window underlay, long *pTransparent)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   return (*t->GetTransparentIndexSUN)(dpy, overlay, underlay, pTransparent);
}



/*** GLX_MESA_copy_sub_buffer ***/

void PUBLIC
glXCopySubBufferMESA(Display *dpy, GLXDrawable drawable, int x, int y, int width, int height)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return;
   (t->CopySubBufferMESA)(dpy, drawable, x, y, width, height);
}



/*** GLX_MESA_release_buffers ***/

Bool PUBLIC
glXReleaseBuffersMESA(Display *dpy, Window w)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   return (t->ReleaseBuffersMESA)(dpy, w);
}



/*** GLX_MESA_pixmap_colormap ***/

GLXPixmap PUBLIC
glXCreateGLXPixmapMESA(Display *dpy, XVisualInfo *visinfo, Pixmap pixmap, Colormap cmap)
{
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t)
      return 0;
   return (t->CreateGLXPixmapMESA)(dpy, visinfo, pixmap, cmap);
}



/*** GLX_MESA_set_3dfx_mode ***/

Bool PUBLIC
glXSet3DfxModeMESA(int mode)
{
   struct _glxapi_table *t;
   Display *dpy = glXGetCurrentDisplay();
   GET_DISPATCH(dpy, t);
   if (!t)
      return False;
   return (t->Set3DfxModeMESA)(mode);
}



/*** GLX_NV_vertex_array_range ***/

void PUBLIC *
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


void PUBLIC
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

GLuint PUBLIC
glXGetAGPOffsetMESA( const GLvoid *pointer )
{
   struct _glxapi_table *t;
   Display *dpy = glXGetCurrentDisplay();
   GET_DISPATCH(dpy, t);
   if (!t)
      return ~0;
   return (t->GetAGPOffsetMESA)(pointer);
}


/*** GLX_MESA_allocate_memory */

void *
glXAllocateMemoryMESA(Display *dpy, int scrn, size_t size,
                      float readfreq, float writefreq, float priority)
{
   /* dummy */
   return NULL;
}

void
glXFreeMemoryMESA(Display *dpy, int scrn, void *pointer)
{
   /* dummy */
}


GLuint
glXGetMemoryOffsetMESA(Display *dpy, int scrn, const void *pointer)
{
   /* dummy */
   return 0;
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
   typedef int (*nop_func)(void);
   nop_func *dispatch = (nop_func *) t;
   GLuint n = _glxapi_get_dispatch_table_size();
   GLuint i;
   for (i = 0; i < n; i++) {
      dispatch[i] = generic_no_op_func;
   }
}


struct name_address_pair {
   const char *Name;
   __GLXextFuncPtr Address;
};

static struct name_address_pair GLX_functions[] = {
   /*** GLX_VERSION_1_0 ***/
   { "glXChooseVisual", (__GLXextFuncPtr) glXChooseVisual },
   { "glXCopyContext", (__GLXextFuncPtr) glXCopyContext },
   { "glXCreateContext", (__GLXextFuncPtr) glXCreateContext },
   { "glXCreateGLXPixmap", (__GLXextFuncPtr) glXCreateGLXPixmap },
   { "glXDestroyContext", (__GLXextFuncPtr) glXDestroyContext },
   { "glXDestroyGLXPixmap", (__GLXextFuncPtr) glXDestroyGLXPixmap },
   { "glXGetConfig", (__GLXextFuncPtr) glXGetConfig },
   { "glXGetCurrentContext", (__GLXextFuncPtr) glXGetCurrentContext },
   { "glXGetCurrentDrawable", (__GLXextFuncPtr) glXGetCurrentDrawable },
   { "glXIsDirect", (__GLXextFuncPtr) glXIsDirect },
   { "glXMakeCurrent", (__GLXextFuncPtr) glXMakeCurrent },
   { "glXQueryExtension", (__GLXextFuncPtr) glXQueryExtension },
   { "glXQueryVersion", (__GLXextFuncPtr) glXQueryVersion },
   { "glXSwapBuffers", (__GLXextFuncPtr) glXSwapBuffers },
   { "glXUseXFont", (__GLXextFuncPtr) glXUseXFont },
   { "glXWaitGL", (__GLXextFuncPtr) glXWaitGL },
   { "glXWaitX", (__GLXextFuncPtr) glXWaitX },

   /*** GLX_VERSION_1_1 ***/
   { "glXGetClientString", (__GLXextFuncPtr) glXGetClientString },
   { "glXQueryExtensionsString", (__GLXextFuncPtr) glXQueryExtensionsString },
   { "glXQueryServerString", (__GLXextFuncPtr) glXQueryServerString },

   /*** GLX_VERSION_1_2 ***/
   { "glXGetCurrentDisplay", (__GLXextFuncPtr) glXGetCurrentDisplay },

   /*** GLX_VERSION_1_3 ***/
   { "glXChooseFBConfig", (__GLXextFuncPtr) glXChooseFBConfig },
   { "glXCreateNewContext", (__GLXextFuncPtr) glXCreateNewContext },
   { "glXCreatePbuffer", (__GLXextFuncPtr) glXCreatePbuffer },
   { "glXCreatePixmap", (__GLXextFuncPtr) glXCreatePixmap },
   { "glXCreateWindow", (__GLXextFuncPtr) glXCreateWindow },
   { "glXDestroyPbuffer", (__GLXextFuncPtr) glXDestroyPbuffer },
   { "glXDestroyPixmap", (__GLXextFuncPtr) glXDestroyPixmap },
   { "glXDestroyWindow", (__GLXextFuncPtr) glXDestroyWindow },
   { "glXGetCurrentReadDrawable", (__GLXextFuncPtr) glXGetCurrentReadDrawable },
   { "glXGetFBConfigAttrib", (__GLXextFuncPtr) glXGetFBConfigAttrib },
   { "glXGetFBConfigs", (__GLXextFuncPtr) glXGetFBConfigs },
   { "glXGetSelectedEvent", (__GLXextFuncPtr) glXGetSelectedEvent },
   { "glXGetVisualFromFBConfig", (__GLXextFuncPtr) glXGetVisualFromFBConfig },
   { "glXMakeContextCurrent", (__GLXextFuncPtr) glXMakeContextCurrent },
   { "glXQueryContext", (__GLXextFuncPtr) glXQueryContext },
   { "glXQueryDrawable", (__GLXextFuncPtr) glXQueryDrawable },
   { "glXSelectEvent", (__GLXextFuncPtr) glXSelectEvent },

   /*** GLX_VERSION_1_4 ***/
   { "glXGetProcAddress", (__GLXextFuncPtr) glXGetProcAddress },

   /*** GLX_SGI_swap_control ***/
   { "glXSwapIntervalSGI", (__GLXextFuncPtr) glXSwapIntervalSGI },

   /*** GLX_SGI_video_sync ***/
   { "glXGetVideoSyncSGI", (__GLXextFuncPtr) glXGetVideoSyncSGI },
   { "glXWaitVideoSyncSGI", (__GLXextFuncPtr) glXWaitVideoSyncSGI },

   /*** GLX_SGI_make_current_read ***/
   { "glXMakeCurrentReadSGI", (__GLXextFuncPtr) glXMakeCurrentReadSGI },
   { "glXGetCurrentReadDrawableSGI", (__GLXextFuncPtr) glXGetCurrentReadDrawableSGI },

   /*** GLX_SGIX_video_source ***/
#if defined(_VL_H)
   { "glXCreateGLXVideoSourceSGIX", (__GLXextFuncPtr) glXCreateGLXVideoSourceSGIX },
   { "glXDestroyGLXVideoSourceSGIX", (__GLXextFuncPtr) glXDestroyGLXVideoSourceSGIX },
#endif

   /*** GLX_EXT_import_context ***/
   { "glXFreeContextEXT", (__GLXextFuncPtr) glXFreeContextEXT },
   { "glXGetContextIDEXT", (__GLXextFuncPtr) glXGetContextIDEXT },
   { "glXGetCurrentDisplayEXT", (__GLXextFuncPtr) glXGetCurrentDisplayEXT },
   { "glXImportContextEXT", (__GLXextFuncPtr) glXImportContextEXT },
   { "glXQueryContextInfoEXT", (__GLXextFuncPtr) glXQueryContextInfoEXT },

   /*** GLX_SGIX_fbconfig ***/
   { "glXGetFBConfigAttribSGIX", (__GLXextFuncPtr) glXGetFBConfigAttribSGIX },
   { "glXChooseFBConfigSGIX", (__GLXextFuncPtr) glXChooseFBConfigSGIX },
   { "glXCreateGLXPixmapWithConfigSGIX", (__GLXextFuncPtr) glXCreateGLXPixmapWithConfigSGIX },
   { "glXCreateContextWithConfigSGIX", (__GLXextFuncPtr) glXCreateContextWithConfigSGIX },
   { "glXGetVisualFromFBConfigSGIX", (__GLXextFuncPtr) glXGetVisualFromFBConfigSGIX },
   { "glXGetFBConfigFromVisualSGIX", (__GLXextFuncPtr) glXGetFBConfigFromVisualSGIX },

   /*** GLX_SGIX_pbuffer ***/
   { "glXCreateGLXPbufferSGIX", (__GLXextFuncPtr) glXCreateGLXPbufferSGIX },
   { "glXDestroyGLXPbufferSGIX", (__GLXextFuncPtr) glXDestroyGLXPbufferSGIX },
   { "glXQueryGLXPbufferSGIX", (__GLXextFuncPtr) glXQueryGLXPbufferSGIX },
   { "glXSelectEventSGIX", (__GLXextFuncPtr) glXSelectEventSGIX },
   { "glXGetSelectedEventSGIX", (__GLXextFuncPtr) glXGetSelectedEventSGIX },

   /*** GLX_SGI_cushion ***/
   { "glXCushionSGI", (__GLXextFuncPtr) glXCushionSGI },

   /*** GLX_SGIX_video_resize ***/
   { "glXBindChannelToWindowSGIX", (__GLXextFuncPtr) glXBindChannelToWindowSGIX },
   { "glXChannelRectSGIX", (__GLXextFuncPtr) glXChannelRectSGIX },
   { "glXQueryChannelRectSGIX", (__GLXextFuncPtr) glXQueryChannelRectSGIX },
   { "glXQueryChannelDeltasSGIX", (__GLXextFuncPtr) glXQueryChannelDeltasSGIX },
   { "glXChannelRectSyncSGIX", (__GLXextFuncPtr) glXChannelRectSyncSGIX },

   /*** GLX_SGIX_dmbuffer **/
#if defined(_DM_BUFFER_H_)
   { "glXAssociateDMPbufferSGIX", (__GLXextFuncPtr) glXAssociateDMPbufferSGIX },
#endif

   /*** GLX_SGIX_swap_group ***/
   { "glXJoinSwapGroupSGIX", (__GLXextFuncPtr) glXJoinSwapGroupSGIX },

   /*** GLX_SGIX_swap_barrier ***/
   { "glXBindSwapBarrierSGIX", (__GLXextFuncPtr) glXBindSwapBarrierSGIX },
   { "glXQueryMaxSwapBarriersSGIX", (__GLXextFuncPtr) glXQueryMaxSwapBarriersSGIX },

   /*** GLX_SUN_get_transparent_index ***/
   { "glXGetTransparentIndexSUN", (__GLXextFuncPtr) glXGetTransparentIndexSUN },

   /*** GLX_MESA_copy_sub_buffer ***/
   { "glXCopySubBufferMESA", (__GLXextFuncPtr) glXCopySubBufferMESA },

   /*** GLX_MESA_pixmap_colormap ***/
   { "glXCreateGLXPixmapMESA", (__GLXextFuncPtr) glXCreateGLXPixmapMESA },

   /*** GLX_MESA_release_buffers ***/
   { "glXReleaseBuffersMESA", (__GLXextFuncPtr) glXReleaseBuffersMESA },

   /*** GLX_MESA_set_3dfx_mode ***/
   { "glXSet3DfxModeMESA", (__GLXextFuncPtr) glXSet3DfxModeMESA },

   /*** GLX_ARB_get_proc_address ***/
   { "glXGetProcAddressARB", (__GLXextFuncPtr) glXGetProcAddressARB },

   /*** GLX_NV_vertex_array_range ***/
   { "glXAllocateMemoryNV", (__GLXextFuncPtr) glXAllocateMemoryNV },
   { "glXFreeMemoryNV", (__GLXextFuncPtr) glXFreeMemoryNV },

   /*** GLX_MESA_agp_offset ***/
   { "glXGetAGPOffsetMESA", (__GLXextFuncPtr) glXGetAGPOffsetMESA },

   /*** GLX_MESA_allocate_memory ***/
   { "glXAllocateMemoryMESA", (__GLXextFuncPtr) glXAllocateMemoryMESA },
   { "glXFreeMemoryMESA", (__GLXextFuncPtr) glXFreeMemoryMESA },
   { "glXGetMemoryOffsetMESA", (__GLXextFuncPtr) glXGetMemoryOffsetMESA },

   { NULL, NULL }   /* end of list */
};



/*
 * Return address of named glX function, or NULL if not found.
 */
__GLXextFuncPtr
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
__GLXextFuncPtr
glXGetProcAddressARB(const GLubyte *procName)
{
   __GLXextFuncPtr f;

   f = _glxapi_get_proc_address((const char *) procName);
   if (f) {
      return f;
   }

   f = (__GLXextFuncPtr) _glapi_get_proc_address((const char *) procName);
   return f;
}


/* GLX 1.4 */
void (*glXGetProcAddress(const GLubyte *procName))()
{
   return glXGetProcAddressARB(procName);
}
