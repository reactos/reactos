
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


#include <assert.h>
#include <GL/glx.h>
#include "realglx.h"
#include "glxapi.h"


struct _glxapi_table *
_real_GetGLXDispatchTable(void)
{
   static struct _glxapi_table glx;

   /* be sure our dispatch table size <= libGL's table */
   {
      GLuint size = sizeof(struct _glxapi_table) / sizeof(void *);
      (void) size;
      assert(_glxapi_get_dispatch_table_size() >= size);
   }

   /* initialize the whole table to no-ops */
   _glxapi_set_no_op_table(&glx);

   /* now initialize the table with the functions I implement */

   /*** GLX_VERSION_1_0 ***/
   glx.ChooseVisual = _real_glXChooseVisual;
   glx.CopyContext = _real_glXCopyContext;
   glx.CreateContext = _real_glXCreateContext;
   glx.CreateGLXPixmap = _real_glXCreateGLXPixmap;
   glx.DestroyContext = _real_glXDestroyContext;
   glx.DestroyGLXPixmap = _real_glXDestroyGLXPixmap;
   glx.GetConfig = _real_glXGetConfig;
   /*glx.GetCurrentContext = _real_glXGetCurrentContext;*/
   /*glx.GetCurrentDrawable = _real_glXGetCurrentDrawable;*/
   glx.IsDirect = _real_glXIsDirect;
   glx.MakeCurrent = _real_glXMakeCurrent;
   glx.QueryExtension = _real_glXQueryExtension;
   glx.QueryVersion = _real_glXQueryVersion;
   glx.SwapBuffers = _real_glXSwapBuffers;
   glx.UseXFont = _real_glXUseXFont;
   glx.WaitGL = _real_glXWaitGL;
   glx.WaitX = _real_glXWaitX;

   /*** GLX_VERSION_1_1 ***/
   glx.GetClientString = _real_glXGetClientString;
   glx.QueryExtensionsString = _real_glXQueryExtensionsString;
   glx.QueryServerString = _real_glXQueryServerString;

   /*** GLX_VERSION_1_2 ***/
   /*glx.GetCurrentDisplay = _real_glXGetCurrentDisplay;*/

   /*** GLX_VERSION_1_3 ***/
   glx.ChooseFBConfig = _real_glXChooseFBConfig;
   glx.CreateNewContext = _real_glXCreateNewContext;
   glx.CreatePbuffer = _real_glXCreatePbuffer;
   glx.CreatePixmap = _real_glXCreatePixmap;
   glx.CreateWindow = _real_glXCreateWindow;
   glx.DestroyPbuffer = _real_glXDestroyPbuffer;
   glx.DestroyPixmap = _real_glXDestroyPixmap;
   glx.DestroyWindow = _real_glXDestroyWindow;
   /*glx.GetCurrentReadDrawable = _real_glXGetCurrentReadDrawable;*/
   glx.GetFBConfigAttrib = _real_glXGetFBConfigAttrib;
   glx.GetFBConfigs = _real_glXGetFBConfigs;
   glx.GetSelectedEvent = _real_glXGetSelectedEvent;
   glx.GetVisualFromFBConfig = _real_glXGetVisualFromFBConfig;
   glx.MakeContextCurrent = _real_glXMakeContextCurrent;
   glx.QueryContext = _real_glXQueryContext;
   glx.QueryDrawable = _real_glXQueryDrawable;
   glx.SelectEvent = _real_glXSelectEvent;

   /*** GLX_SGI_swap_control ***/
   glx.SwapIntervalSGI = _real_glXSwapIntervalSGI;

   /*** GLX_SGI_video_sync ***/
   glx.GetVideoSyncSGI = _real_glXGetVideoSyncSGI;
   glx.WaitVideoSyncSGI = _real_glXWaitVideoSyncSGI;

   /*** GLX_SGI_make_current_read ***/
   glx.MakeCurrentReadSGI = _real_glXMakeCurrentReadSGI;
   /*glx.GetCurrentReadDrawableSGI = _real_glXGetCurrentReadDrawableSGI;*/

#if defined(_VL_H)
   /*** GLX_SGIX_video_source ***/
   glx.CreateGLXVideoSourceSGIX = _real_glXCreateGLXVideoSourceSGIX;
   glx.DestroyGLXVideoSourceSGIX = _real_glXDestroyGLXVideoSourceSGIX;
#endif

   /*** GLX_EXT_import_context ***/
   glx.FreeContextEXT = _real_glXFreeContextEXT;
   /*glx.GetContextIDEXT = _real_glXGetContextIDEXT;*/
   /*glx.GetCurrentDisplayEXT = _real_glXGetCurrentDisplayEXT;*/
   glx.ImportContextEXT = _real_glXImportContextEXT;
   glx.QueryContextInfoEXT = _real_glXQueryContextInfoEXT;

   /*** GLX_SGIX_fbconfig ***/
   glx.GetFBConfigAttribSGIX = _real_glXGetFBConfigAttribSGIX;
   glx.ChooseFBConfigSGIX = _real_glXChooseFBConfigSGIX;
   glx.CreateGLXPixmapWithConfigSGIX = _real_glXCreateGLXPixmapWithConfigSGIX;
   glx.CreateContextWithConfigSGIX = _real_glXCreateContextWithConfigSGIX;
   glx.GetVisualFromFBConfigSGIX = _real_glXGetVisualFromFBConfigSGIX;
   glx.GetFBConfigFromVisualSGIX = _real_glXGetFBConfigFromVisualSGIX;

   /*** GLX_SGIX_pbuffer ***/
   glx.CreateGLXPbufferSGIX = _real_glXCreateGLXPbufferSGIX;
   glx.DestroyGLXPbufferSGIX = _real_glXDestroyGLXPbufferSGIX;
   glx.QueryGLXPbufferSGIX = _real_glXQueryGLXPbufferSGIX;
   glx.SelectEventSGIX = _real_glXSelectEventSGIX;
   glx.GetSelectedEventSGIX = _real_glXGetSelectedEventSGIX;

   /*** GLX_SGI_cushion ***/
   glx.CushionSGI = _real_glXCushionSGI;

   /*** GLX_SGIX_video_resize ***/
   glx.BindChannelToWindowSGIX = _real_glXBindChannelToWindowSGIX;
   glx.ChannelRectSGIX = _real_glXChannelRectSGIX;
   glx.QueryChannelRectSGIX = _real_glXQueryChannelRectSGIX;
   glx.QueryChannelDeltasSGIX = _real_glXQueryChannelDeltasSGIX;
   glx.ChannelRectSyncSGIX = _real_glXChannelRectSyncSGIX;

#if defined(_DM_BUFFER_H_)
   /*** (GLX_SGIX_dmbuffer ***/
   glx.AssociateDMPbufferSGIX = NULL;
#endif

   /*** GLX_SGIX_swap_group ***/
   glx.JoinSwapGroupSGIX = _real_glXJoinSwapGroupSGIX;

   /*** GLX_SGIX_swap_barrier ***/
   glx.BindSwapBarrierSGIX = _real_glXBindSwapBarrierSGIX;
   glx.QueryMaxSwapBarriersSGIX = _real_glXQueryMaxSwapBarriersSGIX;

   /*** GLX_SUN_get_transparent_index ***/
   glx.GetTransparentIndexSUN = _real_glXGetTransparentIndexSUN;

   /*** GLX_MESA_copy_sub_buffer ***/
   glx.CopySubBufferMESA = _real_glXCopySubBufferMESA;

   /*** GLX_MESA_release_buffers ***/
   glx.ReleaseBuffersMESA = _real_glXReleaseBuffersMESA;

   /*** GLX_MESA_pixmap_colormap ***/
   glx.CreateGLXPixmapMESA = _real_glXCreateGLXPixmapMESA;

   /*** GLX_MESA_set_3dfx_mode ***/
   glx.Set3DfxModeMESA = _real_glXSet3DfxModeMESA;

   /*** GLX_NV_vertex_array_range ***/
   glx.AllocateMemoryNV = _real_glXAllocateMemoryNV;
   glx.FreeMemoryNV = _real_glXFreeMemoryNV;

   /*** GLX_MESA_agp_offset ***/
   glx.GetAGPOffsetMESA = _real_glXGetAGPOffsetMESA;

   return &glx;
}
