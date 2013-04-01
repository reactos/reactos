/**************************************************************************
 *
 * Copyright 2009-2010 VMware, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 *
 **************************************************************************/

/**
 * @file
 * Softpipe/LLVMpipe support.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include <windows.h>

#include "util/u_debug.h"
#include "stw_winsys.h"
#include "gdi/gdi_sw_winsys.h"

#include "softpipe/sp_texture.h"
#include "softpipe/sp_screen.h"
#include "softpipe/sp_public.h"

#ifdef HAVE_LLVMPIPE
#include "llvmpipe/lp_texture.h"
#include "llvmpipe/lp_screen.h"
#include "llvmpipe/lp_public.h"
#endif


static boolean use_llvmpipe = FALSE;


static struct pipe_screen *
gdi_screen_create(void)
{
   const char *default_driver;
   const char *driver;
   struct pipe_screen *screen = NULL;
   struct sw_winsys *winsys;

   winsys = gdi_create_sw_winsys();
   if(!winsys)
      goto no_winsys;

#ifdef HAVE_LLVMPIPE
   default_driver = "llvmpipe";
#else
   default_driver = "softpipe";
#endif

   driver = debug_get_option("GALLIUM_DRIVER", default_driver);

#ifdef HAVE_LLVMPIPE
   if (strcmp(driver, "llvmpipe") == 0) {
      screen = llvmpipe_create_screen( winsys );
   }
#endif

   if (screen == NULL) {
      screen = softpipe_create_screen( winsys );
   } else {
      use_llvmpipe = TRUE;
   }

   if(!screen)
      goto no_screen;

   return screen;
   
no_screen:
   winsys->destroy(winsys);
no_winsys:
   return NULL;
}


static void
gdi_present(struct pipe_screen *screen,
            struct pipe_resource *res,
            HDC hDC)
{
   /* This will fail if any interposing layer (trace, debug, etc) has
    * been introduced between the state-trackers and the pipe driver.
    *
    * Ideally this would get replaced with a call to
    * pipe_screen::flush_frontbuffer().
    *
    * Failing that, it may be necessary for intervening layers to wrap
    * other structs such as this stw_winsys as well...
    */

   struct sw_winsys *winsys = NULL;
   struct sw_displaytarget *dt = NULL;

#ifdef HAVE_LLVMPIPE
   if (use_llvmpipe) {
      winsys = llvmpipe_screen(screen)->winsys;
      dt = llvmpipe_resource(res)->dt;
      gdi_sw_display(winsys, dt, hDC);
      return;
   }
#endif

   winsys = softpipe_screen(screen)->winsys,
   dt = softpipe_resource(res)->dt,
   gdi_sw_display(winsys, dt, hDC);
}


static const struct stw_winsys stw_winsys = {
   &gdi_screen_create,
   &gdi_present,
   NULL, /* get_adapter_luid */
   NULL, /* shared_surface_open */
   NULL, /* shared_surface_close */
   NULL  /* compose */
};


BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
   switch (fdwReason) {
   case DLL_PROCESS_ATTACH:
      stw_init(&stw_winsys);
      stw_init_thread();
      break;

   case DLL_THREAD_ATTACH:
      stw_init_thread();
      break;

   case DLL_THREAD_DETACH:
      stw_cleanup_thread();
      break;

   case DLL_PROCESS_DETACH:
      stw_cleanup_thread();
      stw_cleanup();
      break;
   }
   return TRUE;
}
