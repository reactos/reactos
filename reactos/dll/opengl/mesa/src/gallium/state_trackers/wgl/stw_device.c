/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include <windows.h>

#include "glapi/glapi.h"
#include "util/u_debug.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "pipe/p_screen.h"

#include "stw_device.h"
#include "stw_winsys.h"
#include "stw_pixelformat.h"
#include "stw_icd.h"
#include "stw_tls.h"
#include "stw_framebuffer.h"
#include "stw_st.h"


struct stw_device *stw_dev = NULL;

static int
stw_get_param(struct st_manager *smapi,
              enum st_manager_param param)
{
   switch (param) {
   case ST_MANAGER_BROKEN_INVALIDATE:
      /*
       * Force framebuffer validation on glViewport.
       *
       * Certain applications, like Rhinoceros 4, uses glReadPixels
       * exclusively (never uses SwapBuffers), so framebuffers never get
       * resized unless we check on glViewport.
       */
      return 1;
   default:
      return 0;
   }
}

boolean
stw_init(const struct stw_winsys *stw_winsys)
{
   static struct stw_device stw_dev_storage;
   struct pipe_screen *screen;

   debug_printf("%s\n", __FUNCTION__);
   
   assert(!stw_dev);

   stw_tls_init();

   stw_dev = &stw_dev_storage;
   memset(stw_dev, 0, sizeof(*stw_dev));

#ifdef DEBUG
   stw_dev->memdbg_no = debug_memory_begin();
#endif
   
   stw_dev->stw_winsys = stw_winsys;

   stw_dev->stapi = stw_st_create_api();
   stw_dev->smapi = CALLOC_STRUCT(st_manager);
   if (!stw_dev->stapi || !stw_dev->smapi)
      goto error1;

   screen = stw_winsys->create_screen();
   if(!screen)
      goto error1;

   if(stw_winsys->get_adapter_luid)
      stw_winsys->get_adapter_luid(screen, &stw_dev->AdapterLuid);

   stw_dev->smapi->screen = screen;
   stw_dev->smapi->get_param = stw_get_param;
   stw_dev->screen = screen;

   stw_dev->max_2d_levels =
         screen->get_param(screen, PIPE_CAP_MAX_TEXTURE_2D_LEVELS);
   stw_dev->max_2d_length = 1 << (stw_dev->max_2d_levels - 1);

   pipe_mutex_init( stw_dev->ctx_mutex );
   pipe_mutex_init( stw_dev->fb_mutex );

   stw_dev->ctx_table = handle_table_create();
   if (!stw_dev->ctx_table) {
      goto error1;
   }

   stw_pixelformat_init();

   return TRUE;

error1:
   if (stw_dev->smapi)
      FREE(stw_dev->smapi);
   if (stw_dev->stapi)
      stw_dev->stapi->destroy(stw_dev->stapi);

   stw_dev = NULL;
   return FALSE;
}


boolean
stw_init_thread(void)
{
   return stw_tls_init_thread();
}


void
stw_cleanup_thread(void)
{
   stw_tls_cleanup_thread();
}


void
stw_cleanup(void)
{
   DHGLRC dhglrc;

   debug_printf("%s\n", __FUNCTION__);

   if (!stw_dev)
      return;
   
   /*
    * Abort cleanup if there are still active contexts. In some situations
    * this DLL may be unloaded before the DLL that is using GL contexts is.
    */
   pipe_mutex_lock( stw_dev->ctx_mutex );
   dhglrc = handle_table_get_first_handle(stw_dev->ctx_table);
   pipe_mutex_unlock( stw_dev->ctx_mutex );
   if (dhglrc) {
      debug_printf("%s: contexts still active -- cleanup aborted\n", __FUNCTION__);
      stw_dev = NULL;
      return;
   }

   handle_table_destroy(stw_dev->ctx_table);

   stw_framebuffer_cleanup();
   
   pipe_mutex_destroy( stw_dev->fb_mutex );
   pipe_mutex_destroy( stw_dev->ctx_mutex );
   
   FREE(stw_dev->smapi);
   stw_dev->stapi->destroy(stw_dev->stapi);

   stw_dev->screen->destroy(stw_dev->screen);

   /* glapi is statically linked: we can call the local destroy function. */
#ifdef _GLAPI_NO_EXPORTS
   _glapi_destroy_multithread();
#endif

#ifdef DEBUG
   debug_memory_end(stw_dev->memdbg_no);
#endif

   stw_tls_cleanup();

   stw_dev = NULL;
}


struct stw_context *
stw_lookup_context_locked( DHGLRC dhglrc )
{
   if (dhglrc == 0)
      return NULL;

   if (stw_dev == NULL)
      return NULL;

   return (struct stw_context *) handle_table_get(stw_dev->ctx_table, dhglrc);
}


void APIENTRY
DrvSetCallbackProcs(
   INT nProcs,
   PROC *pProcs )
{
   size_t size;

   if (stw_dev == NULL)
      return;

   size = MIN2(nProcs * sizeof *pProcs, sizeof stw_dev->callbacks);
   memcpy(&stw_dev->callbacks, pProcs, size);

   return;
}


BOOL APIENTRY
DrvValidateVersion(
   ULONG ulVersion )
{
   /* TODO: get the expected version from the winsys */
   return ulVersion == 1;
}
