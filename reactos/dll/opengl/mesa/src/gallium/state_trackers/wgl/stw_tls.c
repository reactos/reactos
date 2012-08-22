/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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

#include "pipe/p_compiler.h"
#include "util/u_memory.h"
#include "stw_tls.h"

static DWORD tlsIndex = TLS_OUT_OF_INDEXES;

boolean
stw_tls_init(void)
{
   tlsIndex = TlsAlloc();
   if (tlsIndex == TLS_OUT_OF_INDEXES) {
      return FALSE;
   }

   return TRUE;
}

static INLINE struct stw_tls_data *
stw_tls_data_create()
{
   struct stw_tls_data *data;

   data = CALLOC_STRUCT(stw_tls_data);
   if (!data)
      goto no_data;

   data->hCallWndProcHook = SetWindowsHookEx(WH_CALLWNDPROC,
                                             stw_call_window_proc,
                                             NULL,
                                             GetCurrentThreadId());
   if(data->hCallWndProcHook == NULL)
      goto no_hook;

   TlsSetValue(tlsIndex, data);

   return data;

no_hook:
   FREE(data);
no_data:
   return NULL;
}

boolean
stw_tls_init_thread(void)
{
   struct stw_tls_data *data;

   if (tlsIndex == TLS_OUT_OF_INDEXES) {
      return FALSE;
   }

   data = stw_tls_data_create();
   if(!data)
      return FALSE;

   return TRUE;
}

void
stw_tls_cleanup_thread(void)
{
   struct stw_tls_data *data;

   if (tlsIndex == TLS_OUT_OF_INDEXES) {
      return;
   }

   data = (struct stw_tls_data *) TlsGetValue(tlsIndex);
   if(data) {
      TlsSetValue(tlsIndex, NULL);
   
      if(data->hCallWndProcHook) {
         UnhookWindowsHookEx(data->hCallWndProcHook);
         data->hCallWndProcHook = NULL;
      }
   
      FREE(data);
   }
}

void
stw_tls_cleanup(void)
{
   if (tlsIndex != TLS_OUT_OF_INDEXES) {
      TlsFree(tlsIndex);
      tlsIndex = TLS_OUT_OF_INDEXES;
   }
}

struct stw_tls_data *
stw_tls_get_data(void)
{
   struct stw_tls_data *data;
   
   if (tlsIndex == TLS_OUT_OF_INDEXES) {
      return NULL;
   }
   
   data = (struct stw_tls_data *) TlsGetValue(tlsIndex);
   if(!data) {
      /* DllMain is called with DLL_THREAD_ATTACH only by threads created after 
       * the DLL is loaded by the process */
      data = stw_tls_data_create();
      if(!data)
         return NULL;
   }

   return data;
}
