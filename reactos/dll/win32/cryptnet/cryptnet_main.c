/*
 * Copyright (C) 2006 Maarten Lankhorst
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include "config.h"
#include "windef.h"
#include "wine/debug.h"
#include "winbase.h"
#include "winnt.h"

WINE_DEFAULT_DEBUG_CHANNEL(cryptnet);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
   TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

   switch (fdwReason) {
      case DLL_WINE_PREATTACH:
         return FALSE;  /* prefer native version */
      case DLL_PROCESS_ATTACH:
         DisableThreadLibraryCalls(hinstDLL);
         break;
      case DLL_PROCESS_DETACH:
         /* Do uninitialisation here */
         break;
      default: break;
   }
   return TRUE;
}

/***********************************************************************
 *    DllRegisterServer (CRYPTNET.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
   FIXME("stub\n");

   return S_OK;
}

/***********************************************************************
 *    DllUnregisterServer (CRYPTNET.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
   FIXME("stub\n");

   return S_OK;
}
